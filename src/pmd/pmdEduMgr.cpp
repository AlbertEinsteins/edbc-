#include "import.hpp"
#include "pd.hpp"
#include "pmd.hpp"
#include "pmdEduMgr.hpp"
#include "common.hpp"


namespace edb
{

int pmdEduMgr::destroyAllInternal()
{
    setDestroyedInternal(true);
    setStopService(true);

    // stop all user edus
    unsigned int timerCnt = 0;
    unsigned int total = getEduCntInternal(EduPrivilege::USER);

    while (total != 0) {
        if (0 == timerCnt % 50) {
            forceEdusInternal(EduPrivilege::USER);
        }
        timerCnt ++;
        sleepMillis(100);
        total = getEduCntInternal(EduPrivilege::USER);
    }

    // stop system edus
    timerCnt = 0;
    total = getEduCntInternal(EduPrivilege::ALL);

    while (total != 0) {
        if (0 == timerCnt % 50) {
            forceEdusInternal(EduPrivilege::ALL);
        }
        timerCnt ++;
        sleepMillis(100);
        total = getEduCntInternal(EduPrivilege::ALL);
    }


    return EDB_OK;
}


int pmdEduMgr::forceUserEdu(edu_id_t eid)
{
    int rc = EDB_OK;
    latch_.wLock();

    if (isSystemEid(eid)) {
        PD_LOG(PdLevel::ERROR, "System edu %d can not be forced.", eid);
        rc = EDB_FORCE_SYSTEM_EDU_ERROR;
        goto error;
    }

    for (auto it = runQueue_.begin(); it != runQueue_.end(); it++) {
        if (it->second->getId() == eid) {
            it->second->force();
            goto done;
        }
    }

    for (auto it = idleQueue_.begin(); it != idleQueue_.end(); it++) {
        if (it->second->getId() == eid) {
            it->second->force();
            goto done;
        }
    }

done:
    latch_.wUnlock();
    return rc;
error:
    goto done;
}


int pmdEduMgr::forceEdusInternal(EduPrivilege prop)
{
    int rc = EDB_OK;
    latch_.wLock();

    for (auto it = runQueue_.begin(); it != runQueue_.end(); it ++) {
        if ( ((EduPrivilege::SYSTEM & prop) && isSystemEidInternal(it->first))
            || ((EduPrivilege::USER & prop) && !isSystemEidInternal(it->first)) ) {
            it->second->force();
            PD_LOG(PdLevel::DEBUG, "force edu(Id:%lld)", it->first);
        }
    }

    for (auto it = idleQueue_.begin(); it != idleQueue_.end(); it ++) {
        if (EduPrivilege::USER & prop) {
            it->second->force();
        }
    }
    
    latch_.wUnlock();
    return rc;
}


unsigned int pmdEduMgr::getEduCntInternal(EduPrivilege prop)
{
    unsigned cnt = 0;
    latch_.rLock();
    for (auto it = runQueue_.begin(); it != runQueue_.end(); it ++) {
        if ( ((EduPrivilege::SYSTEM & prop) && isSystemEidInternal(it->first))
            || ((EduPrivilege::USER & prop) && !isSystemEidInternal(it->first)) ) {
            cnt ++;
        }
    }

    for (auto it = idleQueue_.begin(); it != idleQueue_.end(); it ++) {
        if (EduPrivilege::USER & prop) {
            cnt ++;
        }
    }

    latch_.rUnlock();
    return cnt;
}

int pmdEduMgr::publishEvent(edu_id_t eid, pmdEduEventType type, bool release, void* data)
{
    int rc = EDB_OK;
    pmdEduCB* ecb = NULL;

    latch_.rLock();

    auto it = runQueue_.find(eid);
    if (it == runQueue_.end()) {

        it = idleQueue_.find(eid);
        if (it == idleQueue_.end()) {
            rc = EDB_SYS;
            goto error;
        }
    }
    ecb = it->second;
    ecb->pushEvent(pmdEduEvent(type, release, data));

done:
    latch_.rUnlock();
    return rc;
error:
    goto done;
}

int pmdEduMgr::waitEvent(edu_id_t eid, pmdEduEvent& event, long long millis)
{
    int rc = EDB_OK;
    pmdEduCB* ecb = NULL;
    latch_.rLock();

    auto it = runQueue_.find(eid);
    if (it == runQueue_.end()) {

        it = idleQueue_.find(eid);
        if (it == idleQueue_.end()) {
            rc = EDB_SYS;
            goto error;
        }
    }

    ecb = it->second;
    if (!ecb->waitEvent(event, millis)) {
        rc = EDB_TIMEOUT;
        goto error;
    }

done:
    latch_.rUnlock();
    return rc;
error:
    goto done;
}

int pmdEduMgr::returnEdu(edu_id_t eid, bool force, bool* destroyed)
{
    int rc = EDB_OK;
    pmdEduCB* ecb = NULL;
    EduType type = EduType::TypeUNKNOWN;
    latch_.rLock();

    auto it = runQueue_.find(eid);
    if (it == runQueue_.end()) {
        it = idleQueue_.find(eid);
        if (it == idleQueue_.end()) {
            rc = EDB_SYS;
            *destroyed = false;
            latch_.rUnlock();
            return rc;
        }
    }

    ecb = it->second;
    type = ecb->getType();
    latch_.rUnlock();

    if (!isPoolable(type) || force || isDestroyed() || 
        size() > (unsigned int)(get_global_kcb()->getMaxPoolSize())) {
        rc = destroyEduInternal(eid);
        if (destroyed) {
            if (EDB_OK == rc || EDB_SYS == rc) {
                *destroyed = true;
            } else {
                *destroyed = false;
            }
        }
    } else {
        rc = deactivateEduInternal(eid);
        if (destroyed) {
            if (EDB_SYS == rc) {
                *destroyed = true;
            } else {
                *destroyed = false;
            }
        }
    }

    return rc;
}


int pmdEduMgr::startEdu(EduType eType, void* arg, edu_id_t* eid)
{
    int rc = EDB_OK;
    edu_id_t eduId = INVALID_EDU_ID;
    pmdEduCB* ecb = NULL;
    auto it = idleQueue_.end();

    if (isStopService()) {
        rc = EDB_STOPSERVICE_ERROR;
        goto done;
    }

    latch_.wLock();

    // check if there exists a idle pooled edu
    if (idleQueue_.empty() || !isPoolable(eType)) {
        latch_.wUnlock();
        rc = createEduInternal(eType, arg, eid);
        if (EDB_OK == rc) {
            goto done;
        }
        goto error;
    }

    // otherwise, will use the pooled edu
    for (it = idleQueue_.begin(); 
        (it != idleQueue_.end() && EduStatus::IDLE != it->second->getStatus());
        it ++);

    if (idleQueue_.end() == it) {
        latch_.wUnlock();
        rc = createEduInternal(eType, arg, eid);
        if (EDB_OK == rc) {
            goto done;
        }
        goto error;
    }

    // here means, we got a idle edu
    eduId = it->first;
    ecb = it->second;
    idleQueue_.erase(eduId);

    // here we got a edu which is must be AGENT
    ecb->setType(eType);
    ecb->setStatus(EduStatus::WAITING);
    runQueue_[eduId] = ecb;
    *eid = eduId;

    latch_.wUnlock();
    // then we should publish a resume event to start edu
    ecb->pushEvent(pmdEduEvent(pmdEduEventType::RESUME, false, arg));
done:
    return rc;
error:
    goto done;
}

int pmdEduMgr::createEduInternal(EduType eType, void* arg, edu_id_t* eid)
{
    int rc = EDB_OK;
    unsigned int prob = 0;
    pmdEduCB* ecb = NULL;
    edu_id_t eId = INVALID_EDU_ID;

    if (isStopService()) {
        rc = EDB_STOPSERVICE_ERROR;
        goto error;
    }

    if (!getEntryPointByType(eType)) {
        PD_LOG(PdLevel::ERROR, "The edu[type:%d] not exist or function is null", eType);
        rc = EDB_INVALID_ARGUMENT;
        prob = 30;
        goto error;
    }

    ecb = new(std::nothrow) pmdEduCB(this, eType);
    if (NULL == ecb) {
        PD_LOG(PdLevel::ERROR, "Failed to create agent control block");
        goto error;
    }

    ecb->setStatus(EduStatus::CREATING);

    // check exists already
    latch_.wLock();
    if (runQueue_.end() != runQueue_.find(global_edu_id_)) {
        latch_.wUnlock();
        prob = 10;
        rc = EDB_SYS;
        goto error;
    }

    if (idleQueue_.end() != idleQueue_.find(global_edu_id_)) {
        latch_.wUnlock();
        prob = 15;
        rc = EDB_SYS;
        goto error;
    }

    ecb->setId(global_edu_id_);
    if (eid) {
        *eid = global_edu_id_;
    }
    runQueue_[global_edu_id_] = ecb;
    eId = global_edu_id_;
    global_edu_id_ ++;
    latch_.wUnlock();

    // try to create a new thread

    try {
        std::thread mainThread(pmdEntryPointProxy, eType, ecb, arg);
        mainThread.detach();
    } catch (std::exception& e) {
        PD_LOG(PdLevel::ERROR, "Error created agent thread, %s", e.what());
        runQueue_.erase(eId);
        prob = 20;
        rc = EDB_SYS;
        goto error;
    }

    ecb->pushEvent(pmdEduEvent(pmdEduEventType::RESUME, false, arg));

done:
    return rc;
error:
    if (ecb) {
        delete ecb;
    }
    PD_LOG(PdLevel::ERROR, "Failed to create new agent, prob = %d", prob);
    goto done;
}


// destroy edu
// set the status either in WAITING or IDLE or CREATING to DESTROY 
int pmdEduMgr::destroyEduInternal(edu_id_t eid)
{
    int rc = EDB_OK;
    pmdEduCB* ecb = NULL;

    latch_.wLock();
    auto it = runQueue_.find(eid);
    if (runQueue_.end() == it) {
        it = idleQueue_.find(eid);
        if (idleQueue_.end() == it) {
            rc = EDB_SYS;
            goto error;
        }

        // check if in IDLE
        ecb = it->second;
        if (!isEduInStatus(ecb, EduStatus::IDLE)) {
            rc = EDB_EDU_INVALID_STATUS;
            goto error;
        }
        ecb->setStatus(EduStatus::DESTROY);
        idleQueue_.erase(eid);
    } else {
        // here we have to check the runQueue_
        ecb = it->second;
        if (!isEduInStatus(ecb, EduStatus::WAITING) &&
            !isEduInStatus(ecb, EduStatus::CREATING)) {
            rc = EDB_EDU_INVALID_STATUS;
            goto error;
        }
        ecb->setStatus(EduStatus::DESTROY);
        runQueue_.erase(eid);
    }

    // clear tid2eidMap_
    for (auto it = tid2eidMap_.begin(); 
        it != tid2eidMap_.end(); it ++) {
        if (it->second == eid) {
            tid2eidMap_.erase(it);
            break;
        }
    }

    if (ecb) {
        delete ecb;
    }

done:
    latch_.wUnlock();
    return rc;
error:
    goto done;
}


// The function set the ecb of status either in CREATING/WAITING/IDLE to RUNNING status
// and then move the ecb to runQueue_
// the CREATING/WAITING in runQueue_
// the IDLE in idleQueue_
// @param:
//      -eid, edu id
// @return:
//      -EDB_OK, success
//      -EDB_SYS, edu is not found or the edu type is not AGENT
//      -EDB_EDU_INVALID_STATUS, edu is found, but not in an expected status
int pmdEduMgr::activateEdu(edu_id_t eid)
{
    int rc = EDB_OK;
    pmdEduCB* ecb = NULL;

    latch_.wLock();
    auto it = runQueue_.find(eid);
    if (runQueue_.end() == it) {
        it = idleQueue_.find(eid);
        if (idleQueue_.end() == it) {
            rc = EDB_SYS;
            goto error;
        }

        ecb = it->second;
        // here means, we got a ecb in idleQueue_
        if (!isEduInStatus(ecb, EduStatus::IDLE)) {
            rc = EDB_EDU_INVALID_STATUS;
            goto error;
        }
        // move from idle to runQueue
        ecb->setStatus(EduStatus::RUNNING);
        runQueue_[eid] = ecb;
        idleQueue_.erase(eid);
        goto done;
    }
    ecb = it->second;
    // here, we found ecb in runQueue_
    if (isEduInStatus(ecb, EduStatus::RUNNING)) {
        goto done;
    }

    // check status
    if (!isEduInStatus(ecb, EduStatus::CREATING) && 
        !isEduInStatus(ecb, EduStatus::WAITING)) {
        rc = EDB_EDU_INVALID_STATUS;
        goto error;
    }

    ecb = it->second;
    ecb->setStatus(EduStatus::RUNNING);
done:
    latch_.wUnlock();
    return rc;
error:
    goto done;
}

int pmdEduMgr::waitEdu(edu_id_t eid)
{
    int rc = EDB_OK;
    pmdEduCB* ecb = NULL;

    latch_.rLock();
    auto it = runQueue_.find(eid);
    if (runQueue_.end() == it) {
        rc = EDB_SYS;
        goto error;
    }

    ecb = it->second;
    if (isEduInStatus(ecb, EduStatus::WAITING)) {
        goto done;
    }

    if (!isEduInStatus(ecb, EduStatus::RUNNING)) {
        rc = EDB_EDU_INVALID_STATUS;
        goto error;
    }

    ecb->setStatus(EduStatus::WAITING);
done:
    latch_.rUnlock();
    return rc;
error:
    goto done;
}



// set the WAITING/CREATING edu to IDLE status
// then move the edu from runQueue_ to idleQueue_ (aka pooled)
int pmdEduMgr::deactivateEduInternal(edu_id_t eid)
{
    int rc = EDB_OK;
    pmdEduCB* ecb = NULL;

    latch_.wLock();
    auto it = runQueue_.find(eid);
    if (runQueue_.end() == it) {
        it = idleQueue_.find(eid);
        if (idleQueue_.end() == it) {
            rc = EDB_SYS;
            goto error;
        }
        goto done;
    }

    ecb = it->second;
    // check if alread in IDLE
    if (isEduInStatus(ecb, EduStatus::IDLE)) {
        goto done;
    }

    if (!isEduInStatus(ecb, EduStatus::WAITING) ||
        !isEduInStatus(ecb, EduStatus::CREATING)) {
        rc = EDB_EDU_INVALID_STATUS;
        goto error;
    }

    runQueue_.erase(eid);
    ecb->setStatus(EduStatus::IDLE);
    idleQueue_[eid] = ecb;
done:
    latch_.wUnlock();
    return rc;
error:
    goto done;
}



pmdEduCB* pmdEduMgr::getEdu(pthread_t tid)
{
    edu_id_t eid = INVALID_EDU_ID;
    pmdEduCB* ecb = NULL;
    auto it = tid2eidMap_.find(tid);
    auto it2 = runQueue_.find(eid);

    latch_.rLock();
    it = tid2eidMap_.find(tid);
    if (tid2eidMap_.end() == it) {
        goto done;
    }

    eid = it->second;
    it2 = runQueue_.find(eid);
    if (runQueue_.end() != it2) {
        ecb = it2->second;
        goto done;
    }

    it2 = idleQueue_.find(eid);
    if (idleQueue_.end() != it2) {
        ecb = it2->second;
        goto done;
    }

done:
    latch_.rUnlock();
    return ecb;
}

pmdEduCB* pmdEduMgr::getCurrentEdu()
{
    return getEdu(getCurrentTid());
}

pmdEduCB* pmdEduMgr::getEduByEid(edu_id_t eid)
{
    pmdEduCB* ecb = NULL;
    latch_.rLock();
    auto it = runQueue_.find(eid);
    if (runQueue_.end() == it) {
        it = idleQueue_.find(eid);
        if (idleQueue_.end() == it) {
            goto done;
        }
    }

    ecb = it->second;
done:
    latch_.rUnlock();
    return ecb;
}

void pmdEduMgr::setEdu(pthread_t tid, edu_id_t eid)
{
    latch_.wLock();
    tid2eidMap_[tid] = eid;
    latch_.wUnlock();
}


}