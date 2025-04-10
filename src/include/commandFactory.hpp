#ifndef _COMMAND_FACTORY
#define _COMMAND_FACTORY

#include "command.hpp"

namespace edb {
class CommandFactory 
{

public:
    CommandFactory() {};
    ~CommandFactory() {};

    void addCommand(std::string cmdName, ICommand *processor);

    ICommand* getCommandProcessor(const std::string& type);

private:
    std::unordered_map<std::string, ICommand*> commandMap_;
};


}


#endif