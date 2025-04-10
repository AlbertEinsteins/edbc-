#ifndef _MSG_HPP
#define _MSG_HPP

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#define RTN_CODE_STATE_OK 1

namespace edb
{

const static int OP_REPLY = 1;
const static int OP_INSERT = 2;
const static int OP_DELETE = 3;
const static int OP_QUERY = 4;
const static int OP_COMMAND = 5;
const static int OP_DISCONNECT = 6;
const static int OP_CONNECT = 7;
const static int OP_SNAPSHOT = 8;


struct MsgHeader
{
    int msgLen_;
    int opCode_;
};

struct MsgReply
{
    MsgHeader header_;
    int rtnCode_;
    int numRecords_;
    char data_[0];
};

struct MsgInsert
{
    MsgHeader header_;
    int numRecords_;
    char data_[0];
};

struct MsgDelete
{
    MsgHeader header_;
    char key_[0];
};

struct MsgQuery
{
    MsgHeader header_;
    char key_[0];
};

struct MsgCommand
{
    MsgHeader header_;
    int numArgs_;
    char data_[0];
};


class MsgBuilder
{
private:
    MsgBuilder() {};

protected:
    static int checkBuffer(char** ppBuffer, int* pSize, int length);

public:
    // static methods
    // build msg from rtnJson to *ppBuffer
    static int buildReplyMsg(char** ppBuffer, int* pSize, int rtnCode, json* rtnJson);
    // extract msg from pBuffer to the other params
    static int extractReplyMsg(char* pBuffer, int& rtnCode, int& numRecords, const char** ppObjStart);
    
    // build msg from obj to *ppBuffer
    static int buildInsertMsg(char** ppBuffer, int* pSize, json& obj);
    // build msg from vector<obj> to *ppBuffer
    static int buildInsertMsg(char** ppBuffer, int* pSize, std::vector<json *>& objList);

    static int extractInsertMsg(char* pBuffer, int& numRecords, const char** ppObj);
    
    // build msg from json obj to *ppBuffer
    static int buildDeleteMsg(char** ppBuffer, int* pSize, json& obj);

    // extract msg from pBuffer to json obj
    static int extractDeleteMsg(char* pBuffer, json& obj);

    // extract msg from pBuffer to json obj
    static int extractQueryMsg(char* pBuffer, json& obj);
     
    // build msg from obj to *ppBuffer
    static int buildQueryMsg(char** ppBuffer, int* pSize, json& obj);
    
    // build msg from obj to *ppBuffer
    static int buildCommandMsg(char** ppBuffer, int* pSize, json& obj);

    static int buildCommandMsg(char** ppBuffer, int* pSize, std::vector<json *>& objs);

    static int extractCommandMsg(char* pBuffer, int& numArgs, const char** ppObj);
};

}



#endif