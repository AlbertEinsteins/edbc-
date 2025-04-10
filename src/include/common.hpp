#ifndef _COMMON_HPP
#define _COMMON_HPP

#include <import.hpp>

namespace edb {
const static int EDB_OK = 0;
const static int EDB_IO = -1;
const static int EDB_INVALIDARG = -2;
const static int EDB_NETWORK = -3;
const static int EDB_TIMEOUT = -4;
const static int EDB_NETWORK_CLOSE = -5;
const static int EDB_RECV_DATA_LENGTH_ERROR = -6;
const static int EDB_INVALID_RECORD = -7;
const static int EBD_MSG_BUILD_FAILED = -8;
const static int EDB_SOCK_INIT_FAILED = -9;
const static int EDB_SOCK_CONNECT_FAILED = -10;
const static int EDB_SOCK_NOT_CONNECTED = -11;
const static int EDB_SYS = -12;
const static int EDB_APP_FORCED = -13;
const static int EDB_FORCE_SYSTEM_EDU_ERROR = -14;
const static int EDB_STOPSERVICE_ERROR = -15;
const static int EDB_INVALID_ARGUMENT = -16;
const static int EDB_EDU_INVALID_STATUS = -17;
const static int EDB_OOM = -18;
const static int EDB_PERM = -19;
// define the pmd constant of the type of cmd
const static int EDB_PMD_HELP_ONLY = -20;
const static int EDB_RECORD_ID_NOT_EXIST = -21;
const static int EDB_INDEX_EXIST = -22;





// define the constant of client
const static int CMD_BUFFER_SIZE = 1 << 9;

// define the constant of SERVER
const static int AGENT_RECEIVE_BUFFER_SIZE = 1 << 12;
const static int EDB_PAGE_SIZE = 1 << 12;

using page_id_t = uint64_t;
using slot_id_t = uint64_t;

const static int INVALID_PAGEID = 0xFFFFFFFF;
const static int INVALID_SLOTID = 0xFFFFFFFF;

}



#endif
