#ifndef _DMSRID_HPP
#define _DMSRID_HPP

#include "import.hpp"
#include "common.hpp"

namespace edb 
{

struct RID
{
    page_id_t pid_;
    slot_id_t sid_;
};

}

#endif
