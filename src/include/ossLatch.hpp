#ifndef _OSSLATCH_HPP
#define _OSSLATCH_HPP

#include <pthread.h>

namespace edb {

enum OSS_LATCH_MODE
{
    SHARED,
    EXCLUSIVE
};

class ossWLatch
{
private:
    pthread_mutex_t mutex_;

public:
    ossWLatch() 
    {  
        pthread_mutex_init(&mutex_, 0);
    }
    ~ossWLatch() 
    {
        pthread_mutex_destroy(&mutex_);
    }

    void lock() 
    {
        pthread_mutex_lock(&mutex_);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mutex_);
    }

    bool tryLock()
    {
        return pthread_mutex_trylock(&mutex_) == 0;
    }
};


class ossRWLatch
{
private:
    pthread_rwlock_t rwlock_;

public:
    ossRWLatch()
    {
        pthread_rwlock_init(&rwlock_, 0);
    }

    ~ossRWLatch()
    {
        pthread_rwlock_destroy(&rwlock_);
    }

    void wLock()
    {
        pthread_rwlock_wrlock(&rwlock_);
    }

    void wUnlock()
    {
        pthread_rwlock_unlock(&rwlock_);
    }

    bool tryWLock()
    {
        return pthread_rwlock_trywrlock(&rwlock_) == 0;
    }

    void rLock()
    {
        pthread_rwlock_rdlock(&rwlock_);
    }

    void rUnlock()
    {
        pthread_rwlock_unlock(&rwlock_);
    }

    bool tryRLock()
    {
        return pthread_rwlock_tryrdlock(&rwlock_) == 0;
    }
};


}

#endif