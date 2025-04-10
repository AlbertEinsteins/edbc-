#include "import.hpp"
#include "common.hpp"
#include "ossSocket.hpp"
#include "pmdEdu.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "pmdEduMgr.hpp"

#define MAX_RETRY 5

namespace edb {

int pmdTCPListenerEntryPoint(pmdEduCB* ecb, __attribute__((unused)) void* arg)
{
    int rc = EDB_OK;
    pmdEduMgr* mgr = ecb->getEduMgr();
    edu_id_t masterEid = ecb->getId();
    unsigned int retry = 0;
    edu_id_t slaveEid = INVALID_EDU_ID;

    while (retry < MAX_RETRY && !edb_is_shutdown()) {
        retry ++;

        // resolve port
        std::string svcName = get_global_kcb()->getSvcName();
        PD_LOG(PdLevel::EVENT, "Listening on port %s", svcName.c_str());

        int port = 0;

        for (size_t idx = 0; idx < svcName.size(); idx++) {
            if (svcName[idx] >= '0' && svcName[idx] <= '9') {
                port = port * 10 + (svcName[idx] - '0');
            } else {
                PD_LOG(PdLevel::ERROR, "Service name error!");
            }
        }

        ossSocket sock(port);
        rc = sock.initSocket();
        if (rc) {
            PD_LOG(PdLevel::ERROR, "Failed to initialize socket");
            goto error;
        }
        rc = sock.bindListen();
        if (rc) {
            PD_LOG(PdLevel::ERROR, "Failed to bind/listen socket");
            goto error;
        }

        if (EDB_OK != (rc = mgr->activateEdu(ecb->getId()))) {
            goto error;
        }

        while (!edb_is_shutdown()) {
            int fd;
            rc = sock.accept(&fd, NULL, NULL);
            if (EDB_TIMEOUT == rc) {
                rc = EDB_OK;
                continue;
            }

            if (rc && edb_is_shutdown()) {
                rc = EDB_OK;
                goto done;
            } else if (rc) {
                PD_LOG(PdLevel::ERROR, "Failed to accept");
                break;
            }
            void *pData = NULL;
            pData = (void *)&fd;
            rc = mgr->startEdu(EduType::AGENT, pData, &slaveEid);
            if (rc) {
                if (EDB_STOPSERVICE_ERROR == rc) {
                    PD_LOG(PdLevel::WARNING, "The server reject new requests");
                } else {
                    PD_LOG(PdLevel::ERROR, "Failed to start a edu agent");
                }

                ossSocket slaveSock(&fd);
                slaveSock.close();
                continue;
            }
        }

        if (EDB_OK != (rc = mgr->waitEdu(masterEid))) {
            goto error;
        }
    } // while (retry ...)

done: 
    return rc;
error:
    switch (rc) {
    case EDB_SYS:
        PD_LOG(PdLevel::SERVER, "System error occurred!");
        break;
    default:
        PD_LOG(PdLevel::SERVER, "Internal error occurred!");
    }
    goto done;
}


}
