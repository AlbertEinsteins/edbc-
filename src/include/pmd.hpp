#ifndef _PMD_HPP
#define _PMD_HPP

#include "pmdOptions.hpp"
#include <string>
#include "pmdEduMgr.hpp"
#include "rtn.hpp"

namespace edb
{

enum edb_status
{
    NORMAL = 0,
    SHUTDOWN,
    PANIC
};

class pmdOptions;

class edb_kcb
{
private:
    std::string dbFilepath_;
    std::string logFilepath_;
    std::string svcName_;
    edb_status status_;
    int maxPoolSize_;

private:
    pmdEduMgr mgr_;
    rtn rtnMgr_;

public:
    edb_kcb() : status_(NORMAL), maxPoolSize_(0) 
    {
    }

    ~edb_kcb()
    {
    }

    int init(pmdOptions *options);

    pmdEduMgr* getEduMgr()
    {
        return &mgr_;
    }

    rtn* getRtnMgr() 
    {
        return &rtnMgr_;
    }

    const std::string getDBFilepath() const 
    {
        return dbFilepath_;
    }
    const std::string getLogFilepath() const
    {
        return logFilepath_;
    }
    const std::string getSvcName() const 
    {
        return svcName_;
    }

    int getMaxPoolSize() const
    {
        return maxPoolSize_;
    }
    edb_status getStatus() const 
    {
        return status_;
    }

    void setDBFilepath(const std::string& filepath) 
    {
        dbFilepath_ = filepath;
    }
    void setLogFilepath(const std::string& filepath) 
    {
        logFilepath_ = filepath;
    }
    void setSvcName(const std::string& svcName)
    {
        svcName_ = svcName;
    }
    void setMaxPoolSize(const int size) 
    {
        maxPoolSize_ = size;
    }
    void setStatus(const edb_status& status)
    {
        status_ = status;
    }


// define a global constant edb_kcb
};


extern std::unique_ptr<edb_kcb> GLOBAL_KCB;

edb_kcb* get_global_kcb();
bool edb_is_normal();
bool edb_is_shutdown();
bool edb_is_panic();
void shutdown_edb();

}//namespace edb



#endif