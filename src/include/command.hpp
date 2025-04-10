#ifndef _COMMAND_HPP
#define _COMMAND_HPP

#include <nlohmann/json.hpp>
#include "import.hpp"
#include "ossSocket.hpp"
#include "common.hpp"

using json = nlohmann::json;

namespace edb {
static const std::string COMMAND_QUIT = "quit";
static const std::string COMMAND_INSERT = "insert";
static const std::string COMMAND_QUERY = "query";
static const std::string COMMAND_DELETE = "delete";
static const std::string COMMAND_HELP = "help";
static const std::string COMMAND_CONNECT = "connect";
static const std::string COMMAND_TEST = "test";
static const std::string COMMAND_SNAPSHOT = "snapshot";

static const int RECV_BUF_SIZE = 1 << 12;
static const int SEND_BUF_SIZE = 1 << 12;


using buildMsgFunc = int (*)(char **ppBuffer, int *pBufSize, json& jsonData);

class ICommand 
{

public:
    virtual int execute(ossSocket& sock, std::vector<std::string>& args);
    int getError(int code);

protected:
    int recvReply(ossSocket& sock);
    int sendOrder(ossSocket& sock, buildMsgFunc build);
    int sendOrder(ossSocket& sock, int opCode);

    virtual int handleReply() { return EDB_OK; }

protected:
    char recvBuf_[RECV_BUF_SIZE];
    char sendBuf_[SEND_BUF_SIZE];
    std::string jsonStr_;
};


class InsertCommand : public ICommand
{
public:
    int execute(ossSocket& sock, std::vector<std::string>& args);

protected:
    int handleReply();
};

class QueryCommand : public ICommand
{
public:
    int execute(ossSocket& sock, std::vector<std::string>& args);

protected:
    int handleReply();
};

class SnapshotCommand : public ICommand
{
public:
    int execute(ossSocket& sock, std::vector<std::string>& args);

protected:
    int handleReply();
};



class DeleteCommand : public ICommand
{
public:
    int execute(ossSocket& sock, std::vector<std::string>& args);

protected:
    int handleReply();
};



class ConnectCommand : public ICommand
{
public:
    int execute(ossSocket& sock, std::vector<std::string>& args);

private:
    std::string address_;
    int port_;
};

class QuitCommand : public ICommand 
{
public:
    int execute(ossSocket& sock, std::vector<std::string>& args);

protected:
    int handleReply();
};

class HelpCommand: public ICommand
{
public:
    int execute(ossSocket& sock, std::vector<std::string>& args);
}; 



}


#endif