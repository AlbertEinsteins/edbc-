#include "import.hpp"
#include "pmdListener.cpp"
#include "ossSocket.hpp"
#include "pmd.hpp"
#include "pd.hpp"
#include "pmdEdu.hpp"
#include "common.hpp"
#include "pmdEduMgr.hpp"




namespace edb
{


struct edb_sig_info
{
    std::string name_;
    int isHandle_;
};

#define MAX_SINGAL_NUM 65

static edb_sig_info sigInfos[] = 
{
    {"Unkown", 0},
    { "SIGHUP", 1 },     //1
   { "SIGINT", 1 },     //2
   { "SIGQUIT", 1 },    //3
   { "SIGILL", 1 },     //4
   { "SIGTRAP", 1 },    //5
   { "SIGABRT", 1 },    //6
   { "SIGBUS", 1 },     //7
   { "SIGFPE", 1 },     //8
   { "SIGKILL", 1 },    //9
   { "SIGUSR1", 0 },    //10
   { "SIGSEGV", 1 },    //11
   { "SIGUSR2", 0 },    //12
   { "SIGPIPE", 1 },    //13
   { "SIGALRM", 0 },    //14
   { "SIGTERM", 1 },    //15
   { "SIGSTKFLT", 0 },  //16
   { "SIGCHLD", 0 },    //17
   { "SIGCONT", 0 },    //18
   { "SIGSTOP", 1 },    //19
   { "SIGTSTP", 0 },    //20
   { "SIGTTIN", 0 },    //21
   { "SIGTTOU", 0 },    //22
   { "SIGURG", 0 },     //23
   { "SIGXCPU", 0 },    //24
   { "SIGXFSZ", 0 },    //25
   { "SIGVTALRM", 0 },  //26
   { "SIGPROF", 0 },    //27
   { "SIGWINCH", 0 },   //28
   { "SIGIO", 0 },      //29
   { "SIGPWR", 1 },     //30
   { "SIGSYS", 1 },     //31
   { "UNKNOW", 0 },     //32
   { "UNKNOW", 0 },     //33
   { "SIGRTMIN", 0 },   //34
   { "SIGRTMIN+1", 0 }, //35
   { "SIGRTMIN+2", 0 }, //36
   { "SIGRTMIN+3", 0 }, //37
   { "SIGRTMIN+4", 0 }, //38
   { "SIGTTMIN+5", 0 }, //39
   { "SIGRTMIN+6", 0 }, //40
   { "SIGRTMIN+7", 0 }, //41
   { "SIGTTMIN+8", 0 }, //42
   { "SIGRTMIN+9", 0 }, //43
   { "SIGRTMIN+10", 0 },//44
   { "SIGRTMIN+11", 0 },//45
   { "SIGRTMIN+12", 0 },//46
   { "SIGRTMIN+13", 0 },//47
   { "SIGRTMIN+14", 0 },//48
   { "SIGRTMIN+15", 0 },//49
   { "SIGRTMAX-14", 0 },//50
   { "SIGRTMAX-13", 0 },//51
   { "SIGRTMAX-12", 0 },//52
   { "SIGRTMAX-11", 0 },//53
   { "SIGRTMAX-10", 0 },//54
   { "SIGRTMAX-9", 0 }, //55
   { "SIGRTMAX-8", 0 }, //56
   { "SIGRTMAX-7", 0 }, //57
   { "SIGRTMAX-6", 0 }, //58
   { "SIGRTMAX-5", 0 }, //59
   { "SIGRTMAX-4", 0 }, //60
   { "SIGRTMAX-3", 0 }, //61
   { "SIGRTMAX-2", 0 }, //62
   { "SIGRTMAX-1", 0 }, //63
   { "SIGRTMAX", 0 },   //64
};


static void signalHandler(int signalNum)
{
    if (signalNum > 0 && signalNum < MAX_SINGAL_NUM) {
        if (sigInfos[signalNum].isHandle_) {
            PD_LOG(PdLevel::ERROR, "singal %d", signalNum);
            shutdown_edb();
        }
    }
}

int setupSigHandler()
{
    int rc = EDB_OK;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_flags = 0;
    sa.sa_handler = signalHandler;
    for (int i = 1; i < MAX_SINGAL_NUM; i++) {
        sigaction(i, &sa, NULL);
    }
    return rc;
}

int resolveCmdParams(int argc, char *argv[])
{
    int rc = EDB_OK;
    pmdOptions options;
    edb_kcb* globalKCB = get_global_kcb();

    rc = options.readFromCmd(argc, argv);
    if (rc) {
        if (EDB_PMD_HELP_ONLY == rc) {
            PD_LOG(PdLevel::ERROR, "Failed to read params from command line, errno = %d\n", rc);
        }
        goto error;
    }
    
    rc = globalKCB->init(&options);
    if (rc) {
        PD_LOG(PdLevel::ERROR, "Failed to init global_kcb from options, errno = %d\n", rc);
        goto error;
    }

done:
    return rc;
error:
    goto done;
}


int Main(int argc, char *argv[])
{
    int rc = EDB_OK;
    edb_kcb* globalKCB = get_global_kcb();
    pmdEduMgr* mgr = globalKCB->getEduMgr();
    edu_id_t eid = INVALID_EDU_ID;

    rc = setupSigHandler();
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to setup signal handler, errno = %d", rc);

    rc = resolveCmdParams(argc, argv);
    if (EDB_PMD_HELP_ONLY == rc) {
        goto done;
    }

    // start the main loop thread
    rc = mgr->startEdu(EduType::TCPLISTENER, NULL, &eid);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to start main tcplistener, rc = %d", rc);
    while (edb_is_normal()) { 
        sleep(1); 
    }

done:
    return rc;
error:
    goto done;
}

}


int main( int argc, char *argv[] )
{
    // edb::ossSocket a;
    // edb::pmdTCPListenerStart();
    return edb::Main(argc, argv);
}