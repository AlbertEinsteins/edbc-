#include "msg.hpp"
#include "pd.hpp"
#include "import.hpp"
#include "common.hpp"

namespace edb
{


int MsgBuilder::checkBuffer(char **ppBuffer, int* pSize, int length)
{
    int rc = EDB_OK;
    if (*pSize < length) {
        char *oldBuffer = *ppBuffer;
        if (length < 0) {
            PD_LOG(PdLevel::ERROR, "invalid length: %d", length);
            rc = EDB_INVALID_ARGUMENT;
            goto error;
        }
        *ppBuffer = (char *)realloc(*ppBuffer, length);
        if (NULL == *ppBuffer) {
            PD_LOG(PdLevel::ERROR, "Failed to allocate memory");
            rc = EDB_OOM;
            *ppBuffer = oldBuffer;
            goto error;
        }
        *pSize = length;        
    }

done:
    return rc;
error:
    goto done;
}


int MsgBuilder::buildReplyMsg(char** ppBuffer, int* pSize, int rtnCode, json* rtnJson)
{
    int rc = EDB_OK;
    int size = sizeof(MsgReply);
    MsgReply* pReply = NULL;

    std::string jsonStr;
    if (rtnJson) {
        jsonStr = rtnJson->dump();
        size += jsonStr.size();
    }

    // check the receive buffer
    rc = checkBuffer(ppBuffer, pSize, size);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to reallocate memory for bytes %d, rc = %d", *pSize, rc);

    pReply = (MsgReply *)*ppBuffer;
    // fill in obj
    pReply->header_.opCode_ = OP_REPLY;
    pReply->header_.msgLen_ = size;

    pReply->numRecords_ = (rtnJson ? 1 : 0);
    pReply->rtnCode_ = rtnCode;
    if (rtnJson) {
        memcpy(pReply->data_, jsonStr.c_str(), jsonStr.size());
    }
done:
    return rc;
error:
    goto done;
}

int MsgBuilder::extractReplyMsg(char* pBuffer, int& rtnCode, int& numRecords, const char** ppObjStart)
{
    int rc = EDB_OK;
    MsgReply* reply = (MsgReply *)pBuffer;

    // check header
    if (reply->header_.msgLen_ < (int)sizeof(MsgReply)) {
        PD_LOG(PdLevel::ERROR, "Invalid reply length");
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }
    if (OP_REPLY != reply->header_.opCode_) {
        PD_LOG(PdLevel::ERROR, "Receive a non-reply msg");
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }

    rtnCode = reply->rtnCode_;
    numRecords = reply->numRecords_;
    // copy data from pBuffer to ppObjStart
    if (reply->data_ && ppObjStart) {
        *ppObjStart = reply->data_;
    }
done:
    return rc;
error:
    goto done;
}

int MsgBuilder::buildInsertMsg(char** ppBuffer, int* pSize, json& json)
{
    int rc = EDB_OK;
    std::string jsonStr = json.dump();
    int size = sizeof(MsgInsert) + jsonStr.size();
    MsgInsert* pMsg = NULL;

    rc = checkBuffer(ppBuffer, pSize, size);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to reallocate memory for size %d, rc = %d", size, rc);

    pMsg = (MsgInsert *)*ppBuffer;
    pMsg->header_.msgLen_ = size;
    pMsg->header_.opCode_ = OP_INSERT;
    pMsg->numRecords_ = 1;

    // copy data
    memcpy(pMsg->data_, jsonStr.c_str(), jsonStr.size());
done:
    return rc;
error:
    goto done;
}

int MsgBuilder::buildInsertMsg(char** ppBuffer, int* pSize, std::vector<json *>& objList)
{
    int rc = EDB_OK;
    int size = sizeof(MsgInsert);
    MsgInsert* pMsg = NULL;
    std::vector<std::string> bytesAll;
    char* ptr = NULL;

    for (auto it = objList.begin(); it != objList.end(); it ++) {
        bytesAll.push_back((*it)->dump());
        size += bytesAll.back().size();
    }
    rc = checkBuffer(ppBuffer, pSize, size);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to reallocate memory for size: %d, rc = %d", size, rc);

    pMsg->header_.msgLen_ = size;
    pMsg->header_.opCode_ = OP_INSERT;
    pMsg->numRecords_ = objList.size();

    ptr = pMsg->data_;
    for (auto it = bytesAll.begin(); it != bytesAll.end(); it ++) {
        memcpy(ptr, it->c_str(), it->size());
        ptr += it->size();
    }
done:
    return rc;
error:
    goto done;
}

int MsgBuilder::extractInsertMsg(char* ppBuffer, int& numRecords, const char** ppObj)
{
    int rc = EDB_OK;
    MsgInsert* iMsg = (MsgInsert *)ppBuffer;

    if (iMsg->header_.msgLen_ < static_cast<int>(sizeof(MsgInsert))) {
        PD_LOG(PdLevel::ERROR, "Received non-valid insert msg, header len %d", iMsg->header_.msgLen_);
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }
    if (iMsg->header_.opCode_ != OP_INSERT) {
        PD_LOG(PdLevel::ERROR, "Received a non-insert msg, type is %d", iMsg->header_.opCode_);
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }

    numRecords = iMsg->numRecords_;
    if (!ppObj) {
        *ppObj = NULL;
    } else {
        *ppObj = iMsg->data_;
    }
done:
    return rc;
error:
    goto done;
}

int MsgBuilder::extractDeleteMsg(char* pBuffer, json& obj)
{
    int rc = EDB_OK;
    MsgDelete* pMsg = (MsgDelete *)pBuffer;

    if (pMsg->header_.msgLen_ < static_cast<int>(sizeof(MsgDelete))) {
        PD_LOG(PdLevel::ERROR, "Received non-valid delete msg header len %d", pMsg->header_.msgLen_);
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }
    if (OP_DELETE != pMsg->header_.opCode_) {
        PD_LOG(PdLevel::ERROR, 
                "Received a non-delete msg error, msg %d, but expected delete msg", pMsg->header_.opCode_);
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }

    obj = json::parse(std::string(pMsg->key_));
done:
    return rc;
error:
    goto done;
}

int MsgBuilder::buildDeleteMsg(char** ppBuffer, int* pSize, json& obj)
{
    int rc = EDB_OK;
    MsgDelete* msg = NULL;
    std::string jsonStr = obj.dump();
    int size = sizeof(MsgDelete) + jsonStr.size();
    
    rc = checkBuffer(ppBuffer, pSize, size);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to reallocate memory for %d, rc = %d", size, rc);

    msg = (MsgDelete *)*ppBuffer;
    msg->header_.msgLen_ = size;
    msg->header_.opCode_ = OP_DELETE;
    
    memcpy(msg->key_, jsonStr.c_str(), jsonStr.size());
done:
    return rc;
error:
    goto done;
}

int MsgBuilder::buildQueryMsg(char** ppBuffer, int* pSize, json& obj)
{
    int rc = EDB_OK;
    MsgQuery* q = NULL;
    std::string jsonStr = obj.dump();
    int sz = sizeof(MsgQuery) + jsonStr.size();
    rc = checkBuffer(ppBuffer, pSize, sz);

    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to reallocate memory for %d, rc = %d", sz, rc);

    q = (MsgQuery *) *ppBuffer;
    q->header_.msgLen_ = sz;
    q->header_.opCode_ = OP_QUERY;

    memcpy(q->key_, jsonStr.c_str(), jsonStr.size());
done:
    return rc;
error:
    goto done;
}


int MsgBuilder::extractQueryMsg(char* pBuffer, json& obj)
{
    int rc = EDB_OK;
    MsgQuery* pMsg = (MsgQuery *) pBuffer;

    if (pMsg->header_.msgLen_ < static_cast<int>(sizeof(MsgQuery))) {
        PD_LOG(PdLevel::ERROR, "Received non-valid delete msg header len %d", pMsg->header_.msgLen_);
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }
    if (OP_QUERY != pMsg->header_.opCode_) {
        PD_LOG(PdLevel::ERROR, 
                "Received a non-query msg error, msg %d, but expected delete msg", pMsg->header_.opCode_);
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }

    obj = json::parse(pMsg->key_);
done:
    return rc;
error:
    goto done;
}   


int MsgBuilder::buildCommandMsg(char** ppBuffer, int* pSize, json& json)
{
    int rc = EDB_OK;
    MsgCommand* cmdMsg = NULL;
    std::string jsonStr = json.dump();
    int size = sizeof(MsgCommand) + jsonStr.size();

    rc = checkBuffer(ppBuffer, pSize, size);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to reallocate memory for %d, rc = %d", size, rc);

    cmdMsg = (MsgCommand *) *ppBuffer;
    cmdMsg->header_.msgLen_ = size;
    cmdMsg->header_.opCode_ = OP_COMMAND;
    cmdMsg->numArgs_ = 1;
    memcpy(cmdMsg->data_, jsonStr.c_str(), jsonStr.size());

done:
    return rc;
error:
    goto done;
}

int MsgBuilder::buildCommandMsg(char** ppBuffer, int* pSize, std::vector<json *>& objs)
{
    int rc = EDB_OK;
    MsgCommand* cmd = NULL;
    int size = sizeof(MsgCommand);
    std::vector<std::string> bytesAll;
    char *ptr = NULL;

    for (auto it = objs.begin(); it != objs.end(); it++) {
        bytesAll.push_back((*it)->dump());
        size += bytesAll.back().size();
    }

    rc = checkBuffer(ppBuffer, pSize, size);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to reallocate memory %d, rc = %d", size, rc);

    cmd = (MsgCommand *) *ppBuffer;
    cmd->header_.msgLen_ = size;
    cmd->header_.opCode_ = OP_COMMAND;
    cmd->numArgs_ = objs.size();

    ptr = cmd->data_;
    for (auto it = bytesAll.begin(); it != bytesAll.end(); it++) {
        memcpy(ptr, (*it).c_str(), (*it).size());
        ptr += (*it).size();
    }
done:
    return rc;
error:
    goto done;
}


int MsgBuilder::extractCommandMsg(char* pBuffer, int& numArgs, const char** ppObj)
{
    int rc = EDB_OK;
    MsgCommand* cmd = (MsgCommand *)pBuffer;

    if (cmd->header_.msgLen_ < static_cast<int>(sizeof(MsgCommand))) {
        PD_LOG(PdLevel::ERROR, "invalid length of msg");
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }

    if (cmd->header_.opCode_ != OP_COMMAND) {
        PD_LOG(PdLevel::ERROR, "non-command msg received %d, expected %d",
            cmd->header_.opCode_, OP_COMMAND);
        rc = EDB_INVALID_ARGUMENT;
        goto error;
    }

    if (numArgs == 0) {
        *ppObj = NULL;
    } else {
        *ppObj = cmd->data_;
    }

done:
    return rc;
error:
    goto done;
}

}