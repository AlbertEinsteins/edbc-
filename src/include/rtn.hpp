#ifndef _RTN_HPP
#define _RTN_HPP


#include <nlohmann/json.hpp>
#include "dms.hpp"

namespace edb {

class rtn
{
public:
    using json = nlohmann::json;

private:
    std::unique_ptr<dmsFile> dmsFile_;
    std::shared_ptr<ixmHashManager> indexManager_;

public:
    rtn();
    ~rtn();
    int init();
    int insert(json& record);
    int find(json& record, json& outRecord);
    int remove(json& record);
};

}


#endif