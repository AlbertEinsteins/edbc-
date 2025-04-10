#include "import.hpp"
#include "common.hpp"
#include "pmdEdu.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "pmdEduMgr.hpp"


namespace edb
{
static std::unordered_map<EduType, std::string> eduType2NameMap;
static std::unordered_map<EduType, EduType> type2SysTypeMap;

static int registEduName(EduType type, std::string& name, bool system)
{
    int rc = EDB_OK;
    auto iter = eduType2NameMap.find(type);
    if (iter != eduType2NameMap.end()) {
        PD_LOG(PdLevel::ERROR, "Failed, edu type conflict(type: %d, %s<->%s)\n",
            (int)type, iter->second.c_str(), name.c_str());
        rc = EDB_SYS;
        goto error;
    }

    eduType2NameMap[type] = name;
    if (system) {
        type2SysTypeMap[type] = type;
    }

done:
    return rc;
error:
    goto done;
}


const std::string getEduName(EduType type)
{
    auto iter = eduType2NameMap.find(type);
    if (iter != eduType2NameMap.end()) {
        return iter->second;
    }

    return "Unknown";
}

bool isSystemEduType(EduType type)
{
    return type2SysTypeMap.find(type) != type2SysTypeMap.end();
}

pmdEduCB::pmdEduCB(pmdEduMgr* mgr, EduType type)
    : 
    type_(type), 
    mgr_(mgr), 
    status_(EduStatus::CREATING), 
    id_(0),
    isForced_(false),
    isDisconnected_(false)
{}

struct EduEntryInfo
{
    EduType type_;
    int registStatus_;
    pmdEntryPoint entryFunc_;
};

static EduEntryInfo makeEntryInfo(EduType type, bool system,
    pmdEntryPoint entryFunc, std::string desp)
{
    return EduEntryInfo {
        .type_ = type,
        .registStatus_ = registEduName(type, desp, system),
        .entryFunc_ = entryFunc
    };
}

pmdEntryPoint getEntryPointByType(EduType type)
{
    pmdEntryPoint res;

    const static EduEntryInfo infos[] = {
        makeEntryInfo(EduType::AGENT, false, pmdAgentEntryPoint, "Agent"),
        makeEntryInfo(EduType::TCPLISTENER, true, pmdTCPListenerEntryPoint, "TcpListener"),
        makeEntryInfo(EduType::TypeUNKNOWN, false, NULL, "Unknown")
    };

    const size_t len = sizeof(infos) / sizeof(EduEntryInfo);
    for (size_t idx = 0; idx < len; idx++) {
        if (infos[idx].type_ == type) {
            return infos[idx].entryFunc_;
        }
    }
    return res;
}

int pmdRecv(char* pBuffer, size_t recvSize, ossSocket* sock, pmdEduCB* ecb)
{
    int rc = EDB_OK;

    while (true) {
        if (ecb->isForced()) {
            rc = EDB_APP_FORCED;
            goto done;
        }
        rc = sock->recv(pBuffer, recvSize);
        if (EDB_TIMEOUT == rc) {
            continue;
        }
        break;
    }
done:
    return rc;
}

int pmdSend(const char* pBuffer, size_t sendSize, ossSocket* sock, pmdEduCB* ecb)
{
    int rc = EDB_OK;
    while (1) {
        if (ecb->isForced()) {
            rc = EDB_APP_FORCED;
            goto done;
        }
        rc = sock->send(pBuffer, sendSize);
        if (EDB_TIMEOUT == rc) {
            continue;
        }
        break;
    }
done:
    return rc;
}


int pmdEntryPointProxy(EduType type, pmdEduCB* ecb, __attribute__((unused)) void* arg)
{
    int rc = EDB_OK;
    edu_id_t eid = ecb->getId();
    pmdEduMgr* mgr = ecb->getEduMgr();
    pmdEduEvent event;
    bool isForced = false;
    bool destroyed = false;

    while (!destroyed) {
        type = ecb->getType();

        if (!ecb->waitEvent(event, 1000)) {
            if (ecb->isForced()) {
                PD_LOG(PdLevel::EVENT, "Edu %lld is forced", eid);
                isForced = true;
            } else {
                continue ;
            }
        }

        if (!isForced && pmdEduEventType::RESUME == event.type_) {
            mgr->waitEdu(eid);

            pmdEntryPoint execFunc = getEntryPointByType(type);
            if (!execFunc) {
                PD_LOG(PdLevel::ERROR, "Edu %lld type %d exec func is NULL",
                    eid, type);
                shutdown_edb();
                rc = EDB_SYS;
            } else {
                rc = execFunc(ecb, event.data_);
            }

            // sanity check
            if (edb_is_normal()) {
                if (isSystemEduType(type)) {
                    PD_LOG(PdLevel::SERVER, "System edu: %lld,  type %s, exit with %d",
                        eid, getEduName(type).c_str(), rc);
                    shutdown_edb();
                } else if (rc) {
                    PD_LOG(PdLevel::WARNING, "Edu %lld, type %s, exit with %d",
                        eid, getEduName(type).c_str(), rc);
                }
            }

            mgr->waitEdu(eid);
        } else if (!isForced && pmdEduEventType::TERMINATE != event.type_) {
            PD_LOG(PdLevel::ERROR, "Receiving the wrong event %d in edu %lld, type %s",
                type, eid, getEduName(type).c_str());
            rc = EDB_SYS;
        } else if (isForced && pmdEduEventType::TERMINATE == event.type_) {
            PD_LOG(PdLevel::WARNING, "Edu %lld, type %s is forced",
                eid, getEduName(type).c_str());
        } 

        // release space
        if (!isForced && event.data_ && event.release_) {
            free(event.data_);
            event.reset();
        }

        rc = mgr->returnEdu(eid, isForced, &destroyed);
        if (rc) {
            PD_LOG(PdLevel::ERROR, "Invalid edu status, %lld, type %s",
                eid, getEduName(type).c_str());
        }
        PD_LOG(PdLevel::DEBUG, "Terminating thread for edu: %lld, type %s",
            eid, getEduName(type).c_str()); 
    }

    return 0;
}


}