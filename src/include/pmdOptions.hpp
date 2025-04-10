#ifndef _PMDOPTIONS_HPP
#define _PMDOPTIONS_HPP

#include <string>
#include <argparse/argparse.hpp>

namespace edb {

const static int DEFAULT_POOLSIZE = 20;
const static std::string DEFAULT_CONFIG_FILENAME = "edb.conf";
const static std::string DEFAULT_LOG_FILENAME = "diag.log";
const static std::string DEFAULT_DB_FILENAME = "edb.data";
const static std::string DEFAULT_SVCNAME = "48127";

class pmdOptions
{
public:
    pmdOptions();
    ~pmdOptions();

public:
    int readFromCmd(int argc, char *argv[]);
    int readConfigFromFile();


    inline std::string getDBPath() const {
        return dbPath_;
    }
    inline std::string getLogPath() const {
        return logPath_;
    }
    inline std::string getConfigPath() const {
        return confPath_;
    }
    inline std::string getServiceName() const {
        return svcName_;
    }
    inline int getMaxPoolSize() const {
        return maxPool_;
    }

protected:
    void setUp(argparse::ArgumentParser& parser);

private:
    std::string dbPath_;
    std::string logPath_;
    std::string confPath_;
    std::string svcName_;
    int maxPool_;

};



}



#endif
