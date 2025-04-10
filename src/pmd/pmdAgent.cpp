#include "pd.hpp"
#include "pmdEdu.hpp"
#include "pmdEduMgr.hpp"
#include "ossSocket.hpp"
#include "pmd.hpp"
#include "msg.hpp"
#include "dms.hpp"
#include <nlohmann/json.hpp>

namespace edb
{

static int ossRoundUptoMultipleX(int x, int y)
{
    return (x + y - 1 - (x + y - 1)%y);
}

using json = nlohmann::json;

int pmdProcessAgentRequest(char* pRecvBuf,
                           int pRecvSize,
                           char** pSendBuf,
                           int* pSndSize,
                           bool* disconnect,
                           __attribute__((unused)) pmdEduCB* ecb)
{
    PD_LOG(PdLevel::DEBUG, "receive msg %s, size %d", pRecvBuf, pRecvSize);
    PD_LOG(PdLevel::DEBUG, "send buf %s, size %d", pSendBuf, *pSndSize);

    // printf("Receive msg: %s\n", pRecvBuf + sizeof(int));
    // *((int *)pSendBuf) = sizeof(int); 
    int rc = EDB_OK;
    unsigned int prob = 0;
    json record;
    json rtnObj;
    const char* pObj = NULL;
    const char* KEY_FIELD_NAME = gKeyFieldName;
    edb_kcb* GLOBAL_KCB = get_global_kcb();
    rtn* rtnMgr = GLOBAL_KCB->getRtnMgr();

    *disconnect = false;

    // extract headerlen
    MsgHeader* header = (MsgHeader *) pRecvBuf;
    int msgLen = header->msgLen_;
    int opCode = header->opCode_;

    if (msgLen < static_cast<int>(sizeof(MsgHeader))) {
        prob = 10;
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }

    try {
        if (OP_INSERT == opCode) {
            int numRecords = 0;
            PD_LOG(PdLevel::DEBUG, "Insert request received");
            rc = MsgBuilder::extractInsertMsg(pRecvBuf, numRecords, &pObj);
            if (rc) {
                PD_LOG(PdLevel::ERROR, "Received wrong insert packet");
                prob = 15;
                rc = EDB_INVALID_ARGUMENT;
                goto error;
            }

            // parse data
            record = json::parse(std::string(pObj));
            printf("Received insert msg : %s, size = %d\n", record.dump().c_str(), numRecords);

            // check _id exists
            if (!record.contains(KEY_FIELD_NAME)) {
                rc = EDB_RECORD_ID_NOT_EXIST;
                PD_LOG(PdLevel::ERROR, "Failed to check {_id} field", rc);
                goto error;
            }

            rc = rtnMgr->insert(record);
            PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to insert record to file, rc = %d", rc);

        } else if (OP_QUERY == opCode) {
            
            PD_LOG(PdLevel::DEBUG, "Query request received");
            rc = MsgBuilder::extractQueryMsg(pRecvBuf, record);
            if (rc) {
                PD_LOG(PdLevel::ERROR, "Received wrong query packet");
                rc = EDB_INVALID_ARGUMENT;
                goto error;
            }

            if (!record.contains(KEY_FIELD_NAME)) {
                rc = EDB_RECORD_ID_NOT_EXIST;
                PD_LOG(PdLevel::ERROR, "Failed to check {_id} field", rc);
                goto error;
            }

            // do search
            rc = rtnMgr->find(record, rtnObj);
            PD_RC_CHECK(rc, PdLevel::DEBUG, "Failed to find record for id = {%s}", 
                record["_id"].dump().c_str());

        } else if (OP_DELETE == opCode) {
            PD_LOG(PdLevel::DEBUG, "Delete request received");
            rc = MsgBuilder::extractDeleteMsg(pRecvBuf, record);
            if (rc) {
                PD_LOG(PdLevel::ERROR, "Received wrong query packet");
                rc = EDB_INVALID_ARGUMENT;
                goto error;
            }

            if (!record.contains(KEY_FIELD_NAME)) {
                rc = EDB_RECORD_ID_NOT_EXIST;
                PD_LOG(PdLevel::ERROR, "Failed to check {_id} field", rc);
                goto error;
            }

            // do remove
            rc = rtnMgr->remove(record);
            PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to find record for id = {%s}", 
                            record["_id"].dump().c_str());
        } else if (OP_SNAPSHOT == opCode) {
            // build a dummy result
            rtnObj["insertTimes"] = 20;
            rtnObj["delTimes"] = 30;
            rtnObj["queryTimes"] = 100;
            rtnObj["serverRunTime"] = 1000;

        } else if (OP_DISCONNECT == opCode) {

        } else {
            rc = EDB_INVALID_ARGUMENT;
            prob = 90;
            goto error;
        }

    } catch (std::exception& ex) {
        PD_LOG(PdLevel::ERROR, "Error occurred when performing operation: %s", ex.what());
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }

done:
    if (!*disconnect) {
        switch (opCode)
        {
        case OP_QUERY:
        case OP_SNAPSHOT:
            MsgBuilder::buildReplyMsg(pSendBuf, pSndSize, rc, &rtnObj);
            break;
        default:
            MsgBuilder::buildReplyMsg(pSendBuf, pSndSize, rc, NULL);
            break;
        }
    }
    
    return rc;
error:
    switch(rc) {
    case EDB_INVALID_ARGUMENT:
        PD_LOG(PdLevel::ERROR, "Failed received a invalid msg, prob = %d", prob);
        break;
    default:
        PD_LOG(PdLevel::ERROR, "System error rc = %d, prob = %d", rc, prob);
        break;
    }
    goto done;
}

int pmdAgentEntryPoint(pmdEduCB* ecb, void* arg)
{
    int rc = EDB_OK;
    unsigned int prob = 0;
    bool disconnect = false;
    char* pRecvBuf = NULL;
    char* pSendBuf = NULL;
    pmdEduMgr* mgr = ecb->getEduMgr();
    edu_id_t eid = ecb->getId();
    int packetLen = 0;

    int recvBufSize = ossRoundUptoMultipleX(AGENT_RECEIVE_BUFFER_SIZE, EDB_PAGE_SIZE);
    int sendBufSize = sizeof(int);

    int fd = *((int *)arg);
    ossSocket sock(&fd);
    sock.disableNagle();

    // allocate memory
    pRecvBuf = new(std::nothrow) char[recvBufSize];
    if (NULL == pRecvBuf) {
        rc = EDB_OOM;
        prob = 10;
        goto error;
    }

    pSendBuf = new(std::nothrow) char[sendBufSize];
    if (NULL == pSendBuf) {
        rc = EDB_OOM;
        prob = 20;
        goto error;
    }

    while (!disconnect) {
        rc = pmdRecv(pRecvBuf, sizeof(int), &sock, ecb);
        if (rc) {
            if (EDB_APP_FORCED == rc) {
                disconnect = true;
                continue;
            }
            prob = 30;
            goto error;
        }

        packetLen = *(int *)pRecvBuf;
        if (packetLen < (int)sizeof(int)) {
            prob = 40;
            rc = EDB_INVALID_ARGUMENT;
            goto error;
        }

        if (recvBufSize < packetLen + 1) {
            PD_LOG(PdLevel::DEBUG, "The Receiving buffer is too small; %d vs %d, increasing...",
                recvBufSize, packetLen + 1);
            int newSize = ossRoundUptoMultipleX(packetLen + 1, EDB_PAGE_SIZE);
            if (newSize < 0) {
                prob = 50;
                rc = EDB_INVALID_ARGUMENT;
                goto error;
            }
            delete[] pRecvBuf;
            pRecvBuf = new(std::nothrow) char[newSize];
            if (NULL == pRecvBuf) {
                prob = 60;
                rc = EDB_OOM;
                goto error;
            }
            *(int *)pRecvBuf = packetLen;
            recvBufSize = newSize;
        }

        rc = pmdRecv(pRecvBuf + sizeof(int), packetLen - sizeof(int), &sock, ecb);
        if (rc) {
            if (EDB_APP_FORCED == rc) {
                disconnect = true;
                continue;
            }
            prob = 70;
            goto error;
        }

        pRecvBuf[packetLen] = 0;
        if (EDB_OK != (rc = mgr->activateEdu(eid))) {
            goto error;
        }

        rc = pmdProcessAgentRequest(pRecvBuf, packetLen, &pSendBuf, &sendBufSize, 
                                    &disconnect, ecb);
        if (rc) {
            PD_LOG(PdLevel::ERROR, "Error processing agent request, rc = %d", rc);
        }

        if (!disconnect) {
            rc = pmdSend(pSendBuf, sendBufSize, &sock, ecb);
            if (rc) {
                prob = 80;
                goto error;
            }
        }

        if (EDB_OK != (rc = mgr->waitEdu(eid))) {
            goto error;
        }
    }
done:
    if (pRecvBuf) {
        delete[] pRecvBuf;
    }
    if (pSendBuf) {
        delete[] pSendBuf;
    }
    sock.close();
    return rc;
error:
    PD_LOG(PdLevel::SERVER, "error ,rc = %d, prob = %d", rc, prob);
    goto done;
}


}