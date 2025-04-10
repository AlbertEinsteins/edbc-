#ifndef _IXM_HPP
#define _IXM_HPP

#include <nlohmann/json.hpp>
#include <map>
#include "ossLatch.hpp"
#include "dmsRID.hpp"

  
namespace edb
{

struct ixmHashElem
{
    std::string key_;
    RID rid_;

    bool operator==(const ixmHashElem& other)
    {
        return key_ == other.key_;
    }
};

class ixmHashManager
{
public:
    using hash_t = unsigned int;
    using json = nlohmann::json;
    constexpr static int HASH_MAX_SIZE = 1 << 10;
    

private:

    class ixmHashBucket
    {
    private:
        std::multimap<hash_t, ixmHashElem> bucketMap_;
        ossRWLatch rwLatch_;
    public:
        int isIdExist(hash_t hashNum, ixmHashElem& elem);
        int createIndex(hash_t hashNum, ixmHashElem& elem);
        int findIndex(hash_t hashNum, ixmHashElem& elem);
        int removeIndex(hash_t hashNum, ixmHashElem& elem);
    };

    // hash record id
    int hashRecordInternal(json& record, RID& rid,
                    hash_t& hash,           // out
                    ixmHashElem& elem,      // out
                    unsigned int& random);   // out, bucket_idx

private:
    std::vector<ixmHashBucket *> buckets_;
    std::hash<std::string> hashFn_;

public:
    ixmHashManager()
    {}

    ~ixmHashManager()
    {
        for (size_t idx = 0; idx < buckets_.size(); idx ++) {
            if (buckets_[idx]) {
                delete buckets_[idx];
            }
        }
    }

    // init
    int init();
    int isIdExist(json& recordId);
    int createIndex(json& record, RID& recordId);
    int findIndex(json& record, RID& recordId);
    int removeIndex(json& record, RID& recordId);
};




} // namespace edb

#endif