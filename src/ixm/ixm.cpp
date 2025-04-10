#include "ixm.hpp"
#include "dms.hpp"
#include "pd.hpp"

namespace edb
{

// ========= define the ixmHashBucket ================
int ixmHashManager::ixmHashBucket::isIdExist(hash_t hashNum, ixmHashElem& elem)
{ 
    int rc = EDB_OK;

    rwLatch_.rLock();
    auto iters = bucketMap_.equal_range(hashNum);

    for (auto it = iters.first; it != iters.second; it++) {
        if (elem == it->second) {
            rc = EDB_INDEX_EXIST;
            goto error;
        }
    }

done:
    rwLatch_.rUnlock();
    return rc;
error:
    goto done;
}

int ixmHashManager::ixmHashBucket::createIndex(hash_t hashNum, ixmHashElem& elem)
{
    int rc = EDB_OK;
    rwLatch_.wLock();
    bucketMap_.insert(
        { hashNum, elem }
    );
    rwLatch_.wUnlock();
    return rc;
}

int ixmHashManager::ixmHashBucket::findIndex(hash_t hashNum, ixmHashElem& elem)
{
    int rc = EDB_OK;

    rwLatch_.rLock();
    auto iters = bucketMap_.equal_range(hashNum);
    for (auto it = iters.first; it != iters.second; it ++) {
        if (it->second == elem) {
            elem.rid_ = it->second.rid_;
            goto done;
        }
    }

    rc = EDB_RECORD_ID_NOT_EXIST;
    goto error;

done:
    rwLatch_.rUnlock();
    return rc;
error:
    goto done;
}

int ixmHashManager::ixmHashBucket::removeIndex(hash_t hashNum, ixmHashElem& elem)
{
    int rc = EDB_OK;

    rwLatch_.wLock();
    auto iters = bucketMap_.equal_range(hashNum);
    for (auto it = iters.first; it != iters.second; it ++) {
        if (it->second == elem) {
            elem = it->second;
            bucketMap_.erase(it);
            goto done;
        }
    }

    rc = EDB_RECORD_ID_NOT_EXIST;
    goto error;

done:
    rwLatch_.wUnlock();
    return rc;
error:
    goto done;
}


// =========== define ixmHashManager ==============
int ixmHashManager::init()
{
    int rc = EDB_OK;
    ixmHashBucket* temp = nullptr;

    buckets_.resize(HASH_MAX_SIZE);
    for (unsigned int idx = 0; idx < HASH_MAX_SIZE; idx ++) {
        temp = new(std::nothrow) ixmHashBucket;
        if(!temp) {
            rc = EDB_OOM;
            PD_LOG(PdLevel::ERROR, "Faied to allocate memory for bucket, rc = %d", rc);
            goto error;
        }
        buckets_[idx] = temp;
    }
done:
    return rc;
error:
    goto done;
}

int ixmHashManager::hashRecordInternal(json& record, RID& rid,
                                    hash_t& hash,
                                    ixmHashElem& elem,
                                    unsigned int& random)
{
    int rc = EDB_OK;
    const char* pKeyFieldName = gKeyFieldName;
    std::string id;

    // check id exists
    if (!record.contains(pKeyFieldName)) {
        rc = EDB_INVALID_RECORD;
        PD_LOG(PdLevel::ERROR, "The record doesn't contains the {_id} field");
        goto error;
    }

    id = record.at(pKeyFieldName).dump();
    hash = hashFn_(id);
    elem.key_ = id;
    elem.rid_ = rid;
    random = hash % HASH_MAX_SIZE;

done:
    return rc;
error:
    goto done;
}

int ixmHashManager::isIdExist(json& record)
{
    int rc = EDB_OK;
    hash_t hash;
    unsigned int hashIdx = 0;
    RID recordId;
    ixmHashElem elem;
    
    rc = hashRecordInternal(record, recordId, hash, elem, hashIdx);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to hash record, rc = %d", rc);

    rc = buckets_[hashIdx]->isIdExist(hash, elem);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to create index, id exist, rc = %d", rc);

done:
    return rc;
error:
    goto done;
}

int ixmHashManager::createIndex(json& record, RID& recordId)
{
    int rc = EDB_OK;
    hash_t hash;
    unsigned int hashIdx = 0;
    ixmHashElem elem;
    
    rc = hashRecordInternal(record, recordId, hash, elem, hashIdx);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to hash record, rc = %d", rc);

    rc = buckets_[hashIdx]->createIndex(hash, elem);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to create index, rc = %d", rc);
done:
    return rc;
error:
    goto done;
}

int ixmHashManager::findIndex(json& record, RID& recordId)
{
    int rc = EDB_OK;
    hash_t hash;
    unsigned int hashIdx = 0;
    ixmHashElem elem;
    
    rc = hashRecordInternal(record, recordId, hash, elem, hashIdx);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to hash record, rc = %d", rc);

    rc = buckets_[hashIdx]->findIndex(hash, elem);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to find index, rc = %d", rc);
    recordId = elem.rid_;
done:
    return rc;
error:
    goto done;
}

int ixmHashManager::removeIndex(json& record, RID& recordId)
{
    int rc = EDB_OK;
    hash_t hash;
    unsigned int hashIdx = 0;
    ixmHashElem elem;
    
    rc = hashRecordInternal(record, recordId, hash, elem, hashIdx);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to hash record, rc = %d", rc);

    rc = buckets_[hashIdx]->removeIndex(hash, elem);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to remove index, rc = %d", rc);
    recordId = elem.rid_;
done:
    return rc;
error:
    goto done;
}

} // namespace edb