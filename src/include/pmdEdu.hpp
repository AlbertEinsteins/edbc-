#ifndef _PMDEDU_HPP
#define _PMDEDU_HPP


#include "import.hpp"
#include "pmdEduEvent.hpp"
#include "ossQueue.hpp"
#include "ossSocket.hpp"


namespace edb
{

using edu_id_t = unsigned long long;
static const edu_id_t INVALID_EDU_ID = 0;

enum EduType
{
    TCPLISTENER = 0,
    AGENT,
    TypeUNKNOWN,
};

enum EduStatus
{
    CREATING = 0,
    RUNNING,
    WAITING,
    IDLE,
    DESTROY,
    StatusUNKNOWN,
};





class pmdEduMgr;

class pmdEduCB
{
public:
    pmdEduCB(pmdEduMgr *mgr, EduType type);

    inline edu_id_t getId() 
    {
        return id_;
    }

    inline void pushEvent(const pmdEduEvent& data)
    {
        queue_.push(data);
    }

    bool waitEvent(pmdEduEvent& data, long long millis)
    {
        bool isGetMsg = false;
        if (EduStatus::IDLE == status_) {
            status_ = EduStatus::WAITING;
        }

        if (0 > millis) {
            queue_.waitAndPop(data);
            isGetMsg = true;
        } else {
            isGetMsg = queue_.waitForPopMillis(data, millis);
        }

        if (isGetMsg) {
            if (data.type_ == pmdEduEventType::TERMINATE) {
                isDisconnected_ = true;
            } else {
                status_ = EduStatus::RUNNING;
            }
        }
        return isGetMsg;
    }

    inline void force()
    {
        isForced_ = true;
    }
    inline void disconnect()
    {
        isDisconnected_ = true;
    }
    inline EduType getType()
    {
        return type_;
    }
    inline EduStatus getStatus()
    {
        return status_;
    }
    inline void setType(EduType type) 
    {
        type_ = type;
    }
    inline void setId(edu_id_t id) 
    {
        id_ = id;
    }
    inline void setStatus(EduStatus status)
    {
        status_ = status;
    }
    inline bool isForced()
    {
        return isForced_;
    }
    inline pmdEduMgr* getEduMgr()
    {
        return mgr_;
    }


private:
    EduType type_;
    pmdEduMgr *mgr_;
    EduStatus status_;
    edu_id_t id_;
    bool isForced_;
    bool isDisconnected_;
    ossQueue<pmdEduEvent> queue_;

};


using pmdEntryPoint = std::function<int(pmdEduCB*, void*)>;
pmdEntryPoint getEntryPointByType(EduType type);

int pmdAgentEntryPoint(pmdEduCB* ecb, void* arg);
int pmdTCPListenerEntryPoint(pmdEduCB* ecb, void* arg);
int pmdEntryPointProxy(EduType type, pmdEduCB* ecb, void* arg);

int pmdRecv(char *pBuffer, size_t recvSize, ossSocket* sock, pmdEduCB* ecb);
int pmdSend(const char* pBuffer, size_t sendSize, ossSocket* sock, pmdEduCB* ecb);


__attribute__((unused)) static bool isEduInStatus(pmdEduCB* ecb, EduStatus targetStatus)
{
    if (ecb) {
        return ecb->getStatus() == targetStatus;
    }   
    return false;
}
}


#endif