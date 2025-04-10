#include "pmd.hpp"
#include "import.hpp"
#include "common.hpp"

namespace edb
{

std::unique_ptr<edb_kcb> GLOBAL_KCB = std::make_unique<edb_kcb>();

__attribute__((unused)) edb_kcb* get_global_kcb()
{
    return GLOBAL_KCB.get();
}
__attribute__((unused)) bool edb_is_normal() 
{
    return GLOBAL_KCB->getStatus() == edb_status::NORMAL;
}

__attribute__((unused)) bool edb_is_shutdown()
{
    return GLOBAL_KCB->getStatus() == edb_status::SHUTDOWN;
}

__attribute__((unused)) bool edb_is_panic()
{
    return GLOBAL_KCB->getStatus() == edb_status::PANIC;
}

__attribute__((unused)) void shutdown_edb()
{
    GLOBAL_KCB->setStatus(edb_status::SHUTDOWN);
}


int edb_kcb::init(pmdOptions* options)
{
    setStatus(edb_status::NORMAL);
    setDBFilepath(options->getDBPath());
    setLogFilepath(options->getLogPath());
    setSvcName(options->getServiceName());
    setMaxPoolSize(options->getMaxPoolSize());
    return rtnMgr_.init();
}



}