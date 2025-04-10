#include "commandFactory.hpp"
#include "command.hpp"

namespace edb
{

ICommand* CommandFactory::getCommandProcessor(const std::string& type)
{
    ICommand *processor = nullptr;
    auto iter = commandMap_.find(type);
    if (commandMap_.end() == iter) {
        return processor;
    }
    processor = iter->second;
    return processor;
}

void CommandFactory::addCommand(std::string cmdName, ICommand* processor)
{
    commandMap_.insert({cmdName, processor});
}

}