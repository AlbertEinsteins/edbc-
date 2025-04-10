#include "rtn.hpp"
#include "pmd.hpp"
#include "pd.hpp"

namespace edb {

rtn::rtn() 
{ }    

rtn::~rtn()
{ }

int rtn::init()
{
    int rc = EDB_OK;
    edb_kcb* GLOBAL = get_global_kcb();

    try {
        indexManager_ = std::make_shared<ixmHashManager>();
        dmsFile_ = std::make_unique<dmsFile>(indexManager_);
    } catch (const std::bad_alloc& e) {
        rc = EDB_OOM;
        PD_LOG(PdLevel::ERROR, "Failed to allocate memory to dmsFile or indexManager");
        goto error;
    }

    rc = indexManager_->init();
    if (rc) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Failed to initialize indexManager, rc = %d", rc);
        goto error;
    }

    // must init index Manager first
    rc = dmsFile_->initialize(GLOBAL->getDBFilepath().c_str());
    if (rc) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Failed to initialize dmsFile, rc = %d", rc);
        goto error;
    }
    
done:
    return rc;
error:
    goto done;
}

int rtn::insert(json& record)
{
    int rc = EDB_OK;
    RID rid;
    json outRecord;
    rc = indexManager_->isIdExist(record);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to check index, rc = %d", rc);
    
    rc = dmsFile_->insert(record, outRecord, rid);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to insert record, rc = %d", rc);

    rc = indexManager_->createIndex(record, rid);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to create index, rc = %d", rc);
done:
    return rc;
error:
    goto done;
}

int rtn::find(__attribute__((unused)) json& record, 
            __attribute__((unused)) json& outRecord)
{
    int rc = EDB_OK;
    RID rid;

    rc = indexManager_->findIndex(record, rid);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to find index, rc = %d", rc);

    rc = dmsFile_->find(rid, outRecord);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to get row, rc = %d", rc);

done:
    return rc;
error:
    goto done;
}

int rtn::remove(__attribute__((unused)) json& record)
{
    int rc = EDB_OK;
    RID rid;

    rc = indexManager_->removeIndex(record, rid);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to remove index, rc = %d", rc);

    rc = dmsFile_->remove(rid);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to remove data, rc = %d", rc);
done:
    return rc;
error:
    goto done;
}
}