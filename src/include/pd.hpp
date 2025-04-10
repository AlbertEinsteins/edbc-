#ifndef _PD_HPP
#define _PD_HPP

#include <string>
#include "ossLatch.hpp"
#include "ossFileOp.hpp"

#define PD_LOG_STRINGMAX (1 << 12)
#define PD_LOG(level, fmt, ...) \
    do { \
        if (edb::GLOBAL_PD_LEVEL >= level) { \
            edb::pdLog(level, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while (0); \


#define PD_CHECK(cond, code, gotoLabel, level, fmt, ...) \
    do { \
        if (!(cond)) { \
            rc = (code); \
            PD_LOG((level), fmt, ##__VA_ARGS__); \
            goto gotoLabel; \
        } \
    } while (0); \


#define PD_RC_CHECK(rc, level, fmt, ...) \
    do { \
        PD_CHECK((EDB_OK == (rc)), (rc), error, (level), fmt, ##__VA_ARGS__) \
    } while (0); \


#ifdef DEBUG
#define EDB_ASSERT(cond, str) \
    { if (!cond()) { \
        pdAssert((str), __func__, __FILE__, __LINE__); \
      } \
    } \

#define EDB_CHECK(cond, str) \
    { \
        if (!(cond)) { \
            pdCheck((str), __func__, FILE__, __LINE__); \
        } \
    } \

#else
#define EDB_ASSERT(cond, str) { if(!cond()) {}; }
#define EDB_CHECK(cond, str) { if (!cond()) {}; }

#endif

namespace edb {

enum PdLevel
{
    SERVER = 0,
    ERROR,
    EVENT,
    WARNING,
    INFO,
    DEBUG
};

// log level
const static PdLevel PD_DFT_LEVEL = PdLevel::WARNING;
[[maybe_unused]] static PdLevel GLOBAL_PD_LEVEL = PD_DFT_LEVEL;
// temp buffer size
const static int PD_LOG_BUFF_MAX = (1 << 12);

// log format
const static std::string PD_LOG_FORMAT = ""
    "%04d-%02d-%02d-%02d.%2d.%02d.%06d\n"
    "Level: %s\n"
    "PID: %-37dTID:%d\n"
    "Fucntion: %-32sLine:%d\n"
    "File: %s\n"
    "Message:\n"
    "%s\n\n";

// log filename
static std::string GLOBAL_LOG_FILENAME;
static ossWLatch logMutex_;
static ossFileOp logFileOp_;


const std::string getLevelInfo(PdLevel level);
void pdLog(PdLevel level, const char* func, 
    const char* file, unsigned int line, const char* fmt, ...);
void pdLog(PdLevel level, const char* func, 
    const char* file, unsigned int line, std::string& msg);


}

#endif