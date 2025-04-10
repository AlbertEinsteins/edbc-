#include "pd.hpp"
#include "import.hpp"
#include "common.hpp"
#include <fmt/core.h>

namespace edb
{
const static std::unordered_map<int, std::string> pdLevel2String = 
{
    {0, "SERVER"},
    {1, "ERROR"},
    {2, "EVENT"},
    {3, "WARNING"},
    {4, "INFO"},
    {5, "DEBUG"}
};


const std::string getLevelInfo(PdLevel level)
{
    int val = level;
    auto iter = pdLevel2String.find(val);
    if (iter == pdLevel2String.end()) {
        return "Unknow Level";
    }
    return iter->second;
}

static int pdOpenLogFile()
{
    int rc = EDB_OK;
    logFileOp_.close();
    rc = logFileOp_.open(GLOBAL_LOG_FILENAME.c_str());
    if (rc) {
        fmt::print("Failed to open log file {}, errno = {}\n", GLOBAL_LOG_FILENAME, errno);
        goto error;
    }

    logFileOp_.seekToEnd();
done:
    return rc;
error:
    goto done;
}

static int pdWriteLogFile(const char *msg)
{
    int rc = EDB_OK;
    size_t writeSize = strlen(msg);
    logMutex_.lock();
    if (!logFileOp_.isValid()) {
        rc = pdOpenLogFile();
        if (rc) {
            fmt::print("Failed to open log file {}, errno = {}\n", GLOBAL_LOG_FILENAME, errno);
            goto error;
        }
    }
    rc = logFileOp_.write(msg, writeSize);
    if (rc) {
        fmt::print("Failed to write log, errno = {}\n", errno);
        goto error;
    }

done:
    logMutex_.unlock();
    return rc;
error:
    goto done;
}


void pdLog(PdLevel level, const char* func, const char* file, 
    unsigned int line, const char* fmt, ...)
{   
    int rc = EDB_OK;
    if (GLOBAL_PD_LEVEL < level) {
        return ;
    }

    va_list ap;
    char userInfo[PD_LOG_BUFF_MAX];
    char sysInfo[PD_LOG_BUFF_MAX];
    struct tm time;
    struct timeval tv;
    struct timezone tz;
    time_t tt;

    gettimeofday(&tv, &tz);
    tt = tv.tv_sec;
    localtime_r(&tt, &time);

    // write to userInfo
    va_start(ap, fmt);
    vsnprintf(userInfo, PD_LOG_BUFF_MAX, fmt, ap);
    va_end(ap);

    const std::string levelS = pdLevel2String.find(level)->second;
    snprintf(sysInfo, PD_LOG_BUFF_MAX, PD_LOG_FORMAT.c_str(),
        time.tm_year + 1900,
        time.tm_mon + 1,
        time.tm_mday,
        time.tm_hour,
        time.tm_min,
        time.tm_sec,
        tv.tv_usec,
        levelS.c_str(),
        getpid(),
        syscall(SYS_gettid),
        func,
        line,
        file,
        userInfo);
    
    printf("%s\n", sysInfo);
    if (!GLOBAL_LOG_FILENAME.empty()) {
        rc = pdWriteLogFile(sysInfo);
        if (rc) {
            printf("Failed to write log file, errno = %d\n", errno);
        }
    }
    return ;
}


}