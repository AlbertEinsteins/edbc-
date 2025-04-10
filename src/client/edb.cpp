#include "import.hpp"
#include "edb.hpp"
#include "command.hpp"
#include <fmt/core.h>

namespace edb {

const static char SPACE = ' ';
const static char TAB = '\t';
const static char BACK_SLASH = '\\';
const static char NEW_LINE = '\n';

void Edb::start()
{
    std::cout << "Welcome to Edb Shell!" << std::endl;
    std::cout << "ebd help for help, ctrl-c or quit to exit." << std::endl;
    while (!exit_) {
        prompt();
    }
}

void Edb::prompt()
{
    int rc = EDB_OK;
    rc = readInput("edb", 0);
    if (rc) {
        return ;
    }   

    std::string buffer = cmdBuffer_;
    std::vector<std::string> wordList;

    split(buffer, SPACE, wordList);
    int cnt = 0;
    std::string type = "";
    std::vector<std::string> optVec;

    ICommand *cmd = nullptr;
    for (auto& it : wordList) {
        if (cnt == 0) {
            type = it;
            cnt ++;
        } else {
            optVec.push_back(it);
        }
    }

    printf("receive type %s\n", type.c_str());
    cmd = cmdFactory_.getCommandProcessor(type);
    if (nullptr != cmd) {
        int rc = cmd->execute(sock_, optVec);
        if (rc) {
            printf("Error occurred, error code is %d\n", rc);
        }
    } else {
        printf("Failed get processor of command %s\n", type.c_str());
    }
}

int Edb::readInput(const char *msg, int indent)
{
    memset(cmdBuffer_, 0, sizeof(cmdBuffer_));

    for (int i = 0; i < indent; i++) {
        std::cout << TAB;
    }

    std::cout << msg << "> ";
    
    readLine(cmdBuffer_, CMD_BUFFER_SIZE - 1);
    int len = strlen(cmdBuffer_);

    while (cmdBuffer_[len - 1] == BACK_SLASH
        && CMD_BUFFER_SIZE - len > 0) {
        for (int i = 0; i < indent; i++) {
            std::cout << TAB;
        }

        std::cout << "> ";
        readLine(cmdBuffer_ + len, CMD_BUFFER_SIZE - len - 1);
    }

    len = strlen(cmdBuffer_);
    for (int i = 0; i < len; i++) {
        if (cmdBuffer_[i] == TAB) {
            cmdBuffer_[i] = SPACE;
        }
    }
    return EDB_OK;
}

char *Edb::readLine(char *p, int len)
{
    int cnt = 0;
    int ch;
    while ( (ch = getchar()) != NEW_LINE && cnt < len) {
        if (BACK_SLASH == ch) {
            break;
        }
        cmdBuffer_[cnt++] = ch;
    }
    len = strlen(cmdBuffer_);
    p[len] = 0;
    return p;
}


void Edb::split(const std::string& text, char delim, std::vector<std::string>& result)
{
    size_t len = text.size();
    size_t start = 0;
    size_t end = 0;

    for (; start < len; start = end + 1) {
        end = start;
        while (end < len && text[end] != delim) {
            end ++;
        }
        std::string temp = text.substr(start, end - start);
        result.push_back(temp);
    }
}
}




int main()
{
    edb::Edb client;
    client.start();
}