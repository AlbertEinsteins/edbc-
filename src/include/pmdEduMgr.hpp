#ifndef _PMDEDUMGR_HPP
#define _PMDEDUMGR_HPP


#include "import.hpp"
#include "common.hpp"
#include "ossLatch.hpp"
#include "utils.hpp"
#include "pmdEdu.hpp"

namespace edb 
{


enum EduPrivilege
{
    SYSTEM = 1,
    USER,
    ALL
};



class pmdEduMgr
{
private:
    std::map<edu_id_t, pmdEduCB*> runQueue_;
    std::map<edu_id_t, pmdEduCB*> idleQueue_;

    std::map<pthread_t, edu_id_t> tid2eidMap_;

    ossRWLatch latch_;
    edu_id_t global_edu_id_;

    std::map<EduType, edu_id_t> systemEduIdMap_;
    bool stopService_;
    bool isDestroyed_;

public:
    pmdEduMgr()
    : global_edu_id_(1), stopService_(false), isDestroyed_(false)
    {}

    ~pmdEduMgr()
    {
        reset();
    }

    void reset()
    {
        destroyAllInternal();
    }

    unsigned int size()
    {
        unsigned int sz = 0;
        latch_.rLock();
        sz = runQueue_.size() + idleQueue_.size();
        latch_.rUnlock();
        return sz;
    }

    unsigned int idleSize()
    {
        unsigned int sz = 0;
        latch_.rLock();
        sz = idleQueue_.size();
        latch_.rUnlock();
        return sz;
    }

    unsigned int runSize()
    {
        unsigned int sz = 0;
        latch_.rLock();
        sz = runQueue_.size();
        latch_.rUnlock();
        return sz;
    }

    unsigned int systemEduSize()
    {
        unsigned int sz = 0;
        latch_.rLock();
        sz = systemEduIdMap_.size();
        latch_.rUnlock();
        return sz;
    }


    edu_id_t getSystemEid(EduType eduType)
    {
        edu_id_t eid = INVALID_EDU_ID;
        latch_.rLock();
        auto iter = systemEduIdMap_.find(eduType);
        if (iter != systemEduIdMap_.end()) {
            eid = iter->second;
        }
        latch_.rUnlock();
        return eid;
    }

    bool isSystemEid(edu_id_t eid)
    {
        bool isSystem = false;
        latch_.rLock();
        isSystem = isSystemEidInternal(eid);
        latch_.rUnlock();
        return isSystem;
    }

    void registSysEdu(EduType type, edu_id_t eid)
    {
        latch_.wLock();
        systemEduIdMap_[type] = eid;
        latch_.wUnlock();
    }

    bool isStopService()
    {
        return stopService_;
    }

    void setStopService(bool b)
    {
        stopService_ = b;
    }

    bool isDestroyed()
    {
        return isDestroyed_;
    }

    static bool isPoolable(EduType type)
    {
        return EduType::AGENT == type;
    }

private:
    int createEduInternal(EduType eType, void* arg, edu_id_t* eid);
    int destroyAllInternal();
    int forceEdusInternal(EduPrivilege prop=EduPrivilege::ALL);
    unsigned int getEduCntInternal(EduPrivilege prop=EduPrivilege::ALL);
    void setDestroyedInternal(bool b)
    {
        isDestroyed_ = b;
    }

    bool isSystemEidInternal(edu_id_t eid)
    {
        return std::find_if(systemEduIdMap_.begin(), systemEduIdMap_.end(), [&](const auto& item) {
            return item.second == eid;
        }) != systemEduIdMap_.end();
    }

    /*
     *  This function set the status to DESTROY iif the previous status is either 
     *  WAITING or IDLE
     *  @param:
            -eid, the edu id
        @return:
            -EDB_OK, success
            -EDB_SYS, the eid is not found
            -EDB_EDU_INVALID_STATUS, edu is found, but without an expected status
     */
    int destroyEduInternal(edu_id_t eid);

    /*
     * This function will change status of edu to idle, and also push to idle queue
     * representing the edu is pooled
     * @param:
            -eid, edu id
        @return:
            -EDB_OK, success
            -EDB_SYS, edu is not found or the edu type is not AGENT
            -EDB_EDU_INVALID_STATUS, like above 
     */
    int deactivateEduInternal(edu_id_t eid);

public:

    int activateEdu(edu_id_t eid);

    int waitEdu(edu_id_t eid);

    int startEdu(EduType eType, void* arg, edu_id_t* eid);

    int publishEvent(edu_id_t eid, pmdEduEventType eventType, bool release=false, void* data=NULL);

    int waitEvent(edu_id_t eid, pmdEduEvent& event, long long milliSeconds=-1);

    int returnEdu(edu_id_t eid, bool force, bool *destroyed);

    int forceUserEdu(edu_id_t eid);

    pmdEduCB* getEdu(pthread_t tid);
    pmdEduCB* getCurrentEdu();
    pmdEduCB* getEduByEid(edu_id_t eid);
    void setEdu(pthread_t tid, edu_id_t eid);

};





}





#endif