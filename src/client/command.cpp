#include "import.hpp"
#include "command.hpp"
#include "commandFactory.hpp"
#include "ossSocket.hpp"
#include <nlohmann/json.hpp>
#include "pd.hpp"
#include "msg.hpp"


using json = nlohmann::json;

namespace edb {


int ICommand::execute([[maybe_unused]]ossSocket& sock, [[maybe_unused]]std::vector<std::string>& args)
{
    return EDB_OK;
}

int ICommand::getError(int code) 
{
    //TOOD
    return code;
}

int ICommand::recvReply(ossSocket& sock)
{
    int len = 0;
    int ret = EDB_OK;
    memset(recvBuf_, 0, sizeof(recvBuf_));

    if (!sock.isConnected()) {
        return ret;
    }

    while (1) {
        ret = sock.recv(recvBuf_, sizeof(int));
        if (EDB_TIMEOUT == ret) {
            continue;
        }
        if (EDB_NETWORK_CLOSE == ret) {
            return ret;
        }
        break;
    }

    len = *(int *)recvBuf_;
    if (len > RECV_BUF_SIZE) {
        return EDB_RECV_DATA_LENGTH_ERROR;
    }
    // read data
    while (1) {
        ret = sock.recv(recvBuf_ + sizeof(int), len - sizeof(int));
        if (EDB_TIMEOUT == ret) {
            continue;
        }
        if (EDB_NETWORK_CLOSE == ret) {
            return ret;
        }
        break;
    }
    return ret;
}   



int ICommand::sendOrder(ossSocket& sock, buildMsgFunc build)
{
    json jsonData;
    int ret = EDB_OK;

    try {
        jsonData = json::parse(jsonStr_);
    } catch(std::exception& e) {
        PD_LOG(PdLevel::WARNING, "parse %s error", jsonStr_);
        return EDB_INVALID_RECORD;
    }

    memset(sendBuf_, 0, SEND_BUF_SIZE);
    int sz = SEND_BUF_SIZE;
    char *pBuf = sendBuf_;
    ret = build(&pBuf, &sz, jsonData);
    if (ret) {
        return ret;
    }
    ret = sock.send(sendBuf_, *(int *)pBuf);
    if (ret) {
        return ret;
    }
    return ret;
}

int ICommand::sendOrder(ossSocket& sock, [[maybe_unused]] int opCode)
{
    int ret = EDB_OK;
    memset(sendBuf_, 0, SEND_BUF_SIZE);

    MsgHeader* header = (MsgHeader *) sendBuf_;
    header->msgLen_ = sizeof(MsgHeader);
    header->opCode_ = opCode;
    ret = sock.send(sendBuf_, *(int *)sendBuf_);
    return ret;
}


//=================== Query Command ===================
int QueryCommand::handleReply()
{
    MsgReply *reply = (MsgReply *)recvBuf_;
    int code = reply->rtnCode_;
    if (code) {
        return code;
    }

    if (reply->numRecords_) {
        json jsonStr = json::parse(std::string(reply->data_));
        std::cout << jsonStr << std::endl;
    }
   
    return code;
}

int QueryCommand::execute(ossSocket& sock, std::vector<std::string>& args)
{
    int rc = EDB_OK;
    if (args.size() < 1) {
        return EDB_INVALID_ARGUMENT;
    }

    jsonStr_ = args[0];
    if (!sock.isConnected()) {
        return EDB_SOCK_NOT_CONNECTED;
    }
    rc = sendOrder(sock, MsgBuilder::buildQueryMsg);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to send order, rc = %d", rc);
    rc = recvReply(sock);   
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to receive reply, rc = %d", rc);
    rc = handleReply();
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to handle reply, rc = %d", rc);

done:
    return rc;
error:
    goto done;
}

//=================== DeleteCommand ===================
int DeleteCommand::handleReply()
{
    MsgReply *reply = (MsgReply *)recvBuf_;
    int code = reply->rtnCode_;
    return code;
}

int DeleteCommand::execute(ossSocket& sock, std::vector<std::string>& args)
{
    int rc = EDB_OK;
    if (args.size() < 1) {
        return EDB_INVALID_ARGUMENT;
    }

    jsonStr_ = args[0];
    if (!sock.isConnected()) {
        return EDB_SOCK_NOT_CONNECTED;
    }
    
    rc = sendOrder(sock, MsgBuilder::buildDeleteMsg);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to send order, rc = %d", rc);
    rc = recvReply(sock);   
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to receive reply, rc = %d", rc);
    rc = handleReply();
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to handle reply, rc = %d", rc);

done:
    return rc;
error:
    goto done;
}

//==================== Insert Command ==================
int InsertCommand::handleReply()
{
    return EDB_OK;
}

int InsertCommand::execute(ossSocket& sock, std::vector<std::string>& args)
{
    int rc = EDB_OK;
    if (args.size() < 1) {
        return EDB_INVALID_ARGUMENT;
    }

    jsonStr_ = args[0];
    if (!sock.isConnected()) {
        return EDB_SOCK_NOT_CONNECTED;
    }

    rc = sendOrder(sock, MsgBuilder::buildInsertMsg);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to send order, rc = %d", rc);
    rc = recvReply(sock);   
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to receive reply, rc = %d", rc);
    rc = handleReply();
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to handle reply, rc = %d", rc);

done:
    return rc;
error:
    goto done;
}




//==================== Connect Command =================

int ConnectCommand::execute(ossSocket& sock, std::vector<std::string>& args)
{
    int ret = EDB_OK;
    address_ = args[0];
    port_ = atoi(args[1].c_str());

    sock.close();
    sock.setAddress(address_.c_str(), port_);
    ret = sock.initSocket();
    if (ret) {
        return EDB_SOCK_INIT_FAILED;
    }
    ret = sock.connect();
    if (ret) {
        return EDB_SOCK_CONNECT_FAILED;
    }
    sock.disableNagle();
    return ret;
}


//================== QuitCommand ===============
int QuitCommand::handleReply()
{
    return EDB_OK;
}

int QuitCommand::execute(ossSocket& sock, [[maybe_unused]]  std::vector<std::string>& args)
{
    int ret = EDB_OK;
    if (!sock.isConnected()) {
        printf("Network is not connected\n");
        return EDB_SOCK_NOT_CONNECTED;
    }
    ret = sendOrder(sock, OP_DISCONNECT);
    ret = handleReply();
    return ret;
}


//========= Help Command ==================
int HelpCommand::execute([[maybe_unused]] ossSocket& sock, [[maybe_unused]] std::vector<std::string>& args)
{
    printf("List of classes of commands:\n\n");
    printf("%s [server] [port] -- connecting to db server\n", COMMAND_CONNECT.c_str());
    printf("%s -- sending a insert command to db server\n", COMMAND_INSERT.c_str());
    printf("%s -- sending a delete command to db server\n", COMMAND_DELETE.c_str());
    printf("%s -- sending a query command to db server\n", COMMAND_QUERY.c_str());
    printf("%s -- quit\n", COMMAND_QUIT.c_str());
    printf("Type help for help\n");
    return EDB_OK;
}



//============ Snapshot Command ===============
int SnapshotCommand::handleReply()
{
    int rc = EDB_OK;
    MsgReply* reply = (MsgReply *) recvBuf_;
    int rtnCode = reply->rtnCode_;
    if (rtnCode) {
        return rtnCode;
    }

    std::string res = std::string(reply->data_);
    printf("recive result: %s\n", res.c_str());
    json data = json::parse(res);
    printf("insert times is %d\n", data.value("insertTimes", 0));
    printf("del times is %d\n", data.value("delTimes", 0));
    printf("query times is %d\n", data.value("queryTimes", 0));
    printf("server run time is %d\n", data.value("serverRunTime", 0));
    
    return rc;
}

int SnapshotCommand::execute(ossSocket& sock, __attribute__((unused)) std::vector<std::string>& args)
{   
    int rc = EDB_OK;
    if (!sock.isConnected()) {
        return EDB_SOCK_NOT_CONNECTED;
    }

    rc = sendOrder(sock, OP_SNAPSHOT);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to send order, rc = %d", rc);
    rc = recvReply(sock);   
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to receive reply, rc = %d", rc);
    rc = handleReply();
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to handle reply, rc = %d", rc);

done:
    return rc;
error:
    goto done;
}





}

