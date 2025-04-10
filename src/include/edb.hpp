#ifndef _EDB_HPP
#define _EDB_HPP

#include "import.hpp"
#include "common.hpp"
#include "commandFactory.hpp"
#include "command.hpp"
#include "ossSocket.hpp"


namespace edb {

class Edb 
{
public:
    Edb() : exit_(false) {
        cmdFactory_.addCommand(COMMAND_CONNECT, new ConnectCommand());
        cmdFactory_.addCommand(COMMAND_HELP, new HelpCommand());
        cmdFactory_.addCommand(COMMAND_QUIT, new QuitCommand());
        cmdFactory_.addCommand(COMMAND_INSERT, new InsertCommand());
        cmdFactory_.addCommand(COMMAND_QUERY, new QueryCommand());
        cmdFactory_.addCommand(COMMAND_DELETE, new DeleteCommand());
        cmdFactory_.addCommand(COMMAND_SNAPSHOT, new SnapshotCommand());
    };
    ~Edb() {};

    void start();

protected:
    void prompt();

private:
    void split(const std::string& text, char delim, std::vector<std::string>& result);
    char* readLine(char *p, int len);
    int readInput(const char *msg, int indent);

private:
    ossSocket sock_;
    CommandFactory cmdFactory_; 
    char cmdBuffer_[CMD_BUFFER_SIZE];

    bool exit_{false};
};

}

#endif