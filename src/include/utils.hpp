#ifndef _UTILS_HPP
#define _UTILS_HPP

#include "import.hpp"

namespace edb 
{

inline void sleepMicros(unsigned int s)
{
    int rc = EDB_OK;
    struct timespec ts;
    ts.tv_sec = s /1000000;
    ts.tv_nsec = 1000 * (s % 1000000);
    while ((rc = nanosleep(&ts, &ts)) == -1 && EINTR == rc);
}

inline void sleepMillis(unsigned int s)
{
    sleepMicros(s * 1000);
}

inline pid_t getParentPid()
{
    return getppid();
}

inline pid_t getCurrentPid()
{
    return getpid();
}

inline pthread_t getCurrentTid()
{
    return syscall(SYS_gettid);
}



}






#endif