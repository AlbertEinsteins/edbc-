#ifndef _PMDEDUEVENT_HPP
#define _PMDEDUEVENT_HPP


#include "import.hpp"
#include "common.hpp"


namespace edb
{
enum pmdEduEventType
{
    NONE = 0,
    TERMINATE,
    RESUME,
    ACTIVE,
    DEACTIVE,
    MSG,
    TIMEOUT,
    LOCKWAKEUP
};

class pmdEduEvent
{
public:
    pmdEduEvent() : type_(pmdEduEventType::NONE), release_(false), data_(NULL)
    {}
    pmdEduEvent(pmdEduEventType type) : type_(type), release_(false), data_(NULL) 
    {}
    pmdEduEvent(pmdEduEventType type, bool release, void *data) : type_(type), release_(release), data_(data) 
    {}

    void reset()
    {
        type_ = pmdEduEventType::NONE;
        release_ = false;
        data_ = NULL;
    }

public:
    pmdEduEventType type_;
    bool release_;
    void *data_;
};


}

#endif