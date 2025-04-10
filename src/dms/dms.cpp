#include "dms.hpp"
#include "pd.hpp"

namespace edb {

const char* gKeyFieldName = DMS_KEY_FILEIDNAME.c_str();


dmsFile::dmsFile(std::shared_ptr<ixmHashManager> indexMgr) 
    : header_(NULL), pFilename_(NULL), indexManager_(indexMgr)
{ }

dmsFile::~dmsFile()
{
    if (pFilename_) {
        free(pFilename_);
    }
    close();
}


int dmsFile::insert(json& record, 
                    json& outRecord, 
                    RID& rid)
{
    int rc = EDB_OK;
    int pid = 0;
    char* page = nullptr;
    dmsPageHeader* pageHeader = nullptr;
    unsigned int recordSize = 0;
    unsigned int writePos = 0;
    unsigned int slotStart = sizeof(dmsPageHeader);
    dmsRecord recordHeader;

    size_t needSpace = 0;

    // do some validations
    std::string bytes = record.dump();
    recordSize = bytes.size();
    if (recordSize > DMS_MAX_RECORD) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "The Record size is greater than 4MB");
        goto error;
    }

    // check primary key exists
    if (!record.contains(gKeyFieldName)) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Failed to get the key field {_id}");
        goto error;
    }

    // find a page that satisfied the needSpace
    needSpace = recordSize + sizeof(dmsRecord) + sizeof(slot_off_t);

retry:
    rwMutex_.wLock();
    pid = findPage(needSpace);
    
    if (INVALID_PAGEID == pid) {
        rwMutex_.wUnlock();

        extendMutex_.lock();
        rc = extendSegment();
        if (rc) {
            PD_LOG(PdLevel::ERROR, "Failed to extend segment, rc = %d", rc);
            extendMutex_.unlock();
            goto error;
        }

        extendMutex_.unlock();
        
        goto retry;
    }

    // find the target in-memory pointer
    page = pageToOffset(pid);
    if (!page) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Failed to get page from page id, page is null");
        goto error;
    }

    // check pageheader
    pageHeader = reinterpret_cast<dmsPageHeader *>(page);
    if (memcmp(pageHeader->eyeCatcher_, DMS_PAGE_EYECATCHER.c_str(), DMS_PAGE_EYECATCHER_LEN) != 0) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Invalid page header");
        goto error;
    }

    // eusure enough space 
    if (
        // if has holes 
        (pageHeader->freeSpace_ > pageHeader->freeOffset_ - pageHeader->slotOffset_) &&
        (pageHeader->freeOffset_ - pageHeader->slotOffset_ < needSpace) ) {
        reArrangeSpace(page);
    }

    // check if we get enough space
    if (pageHeader->freeSpace_ < needSpace || 
        pageHeader->freeOffset_ - pageHeader->slotOffset_ < needSpace) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Error, we find a page but it doesn't has enough space");
        goto error;
    }

    writePos = pageHeader->freeOffset_ - needSpace;
    recordHeader.size_ = needSpace;
    recordHeader.flag_ = DMS_RECORD_FLAG_NORMAL;

    // set the slot
    *(slot_off_t *)(page + slotStart + pageHeader->numSlots_ * sizeof(slot_off_t))
        = writePos;

    // set record
    memcpy(page + writePos, (char *)&recordHeader, sizeof(dmsRecord));
    memcpy(page + writePos + sizeof(dmsRecord), bytes.c_str(), bytes.size());
    
    // set output
    outRecord = record;
    rid.pid_ = pid;
    rid.sid_ = pageHeader->numSlots_;

    pageHeader->numSlots_ ++;
    pageHeader->slotOffset_ += sizeof(slot_off_t);
    pageHeader->freeOffset_ = writePos;

    updateFreeSpace(pageHeader, -needSpace, pid);

done:
    rwMutex_.wUnlock();    
    return rc;
error:
    goto done;
}

int dmsFile::remove(RID& rid)
{
    int rc = EDB_OK;
    slot_off_t slot = 0;
    char *page = nullptr;
    dmsRecord* recordHeader = nullptr;
    dmsPageHeader* pageHeader = nullptr;

    rwMutex_.wLock();
    page = pageToOffset(rid.pid_);
    if (!page) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Failed to find page, <%u, %u>", rid.pid_, rid.sid_);
        goto error;
    }

    rc = searchSlot(page, rid, slot);
    if (rc) {
        PD_LOG(PdLevel::ERROR, "Failed to search slot, rc = %d", rc);
        goto error;
    }

    if (DMS_SLOT_EMPTY == slot) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "The record had been dropped!");
        goto error;
    }

    // set page meta and record meta
    pageHeader = (dmsPageHeader *)page;
    *(slot_off_t *)(page + sizeof(dmsPageHeader) + rid.sid_ * sizeof(slot_off_t)) = DMS_SLOT_EMPTY;
    
    // set record header
    recordHeader = reinterpret_cast<dmsRecord *>(page + slot);
    recordHeader->flag_ = DMS_RECORD_FLAG_DROPPED;

    // update page meta
    updateFreeSpace(pageHeader, recordHeader->size_, rid.pid_);

done:
    rwMutex_.wUnlock();
    return rc;
error:
    goto done;
}


int dmsFile::find(RID& rid, json& rtnRow)
{
    int rc = EDB_OK;
    dmsRecord* recordHeader = nullptr;
    char* page = nullptr;
    slot_off_t slotOff = 0;
    unsigned int recordSize = 0;

    // check valid rid
    rwMutex_.rLock();
    
    page = pageToOffset(rid.pid_);
    if (!page) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Failed to find page, pid={%d}", rid.pid_);
        goto error;
    }

    rc = searchSlot(page, rid, slotOff);
    if (rc) {
        PD_LOG(PdLevel::ERROR, "Failed to find slot, sid={%d}", rid.sid_);
        goto error;
    }
    
    if (DMS_SLOT_EMPTY == slotOff) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "The record had been dropped");
        goto error;
    }

    recordHeader = reinterpret_cast<dmsRecord *>(page + slotOff);
    if (DMS_RECORD_FLAG_DROPPED == recordHeader->flag_) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "The data has been dropped");
        goto error;
    }
    recordSize = recordHeader->size_ - sizeof(dmsRecord);
    rtnRow = json::parse(std::string(page + slotOff + sizeof(dmsRecord), 0, recordSize));
done:
    return rc;
error:
    goto done;
}

int dmsFile::initialize(const char* pFilename)
{
    offsetType offset = 0;
    int rc = EDB_OK;

    pFilename_ = strdup(pFilename);
    if (NULL == pFilename_) {
        rc = EDB_OOM;
        PD_LOG(PdLevel::ERROR, "Failed to duplicate file name");
        goto error;
    }

    rc = open(pFilename_, OSS_PRIMITIVE_FILE_OP_OPEN_CREAT);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to open file %s, rc = %d", pFilename_, rc);

    do {
        rc = fileOp_.getFileSize(&offset);
        PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to get file size, rc = %d", rc);

        if (0 == offset) {
            rc = initNew();
            PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to initialize file, rc = %d", rc);
        }
    } while (0 == offset);

    // load data
    rc = loadData();
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to load data, rc = %d", rc);

done:
    return rc;
error:
    goto done;
}

// ===== private method definition =========
void dmsFile::updateFreeSpace(dmsPageHeader* header, int changeSize, page_id_t pid)
{
    unsigned int freeSpace = header->freeSpace_;
    auto iters = freeSpaceMap_.equal_range(freeSpace);
    for (auto it = iters.first; it != iters.second; it ++) {
        if (pid == it->second) {
            freeSpaceMap_.erase(it);
            break;
        }
    }

    // increase page free space
    freeSpace += changeSize;
    header->freeSpace_ = freeSpace;
    freeSpaceMap_.insert({ freeSpace, pid });
}

int dmsFile::extendSegment()
{
    int rc = EDB_OK;
    char* data = nullptr;
    int freeMapSize = 0;
    dmsPageHeader header;
    offsetType offset = 0;

    // first, get the size of the file before
    rc = fileOp_.getFileSize(&offset);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to get filesize, rc = %d", rc);

    // extend the file
    rc = extendFile(DMS_FILE_SEGMENT_SIZE);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to extend segment, rc = %d", rc);
    
    //map from original end to new end
    rc = map(offset, DMS_FILE_SEGMENT_SIZE, (void **)&data);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to map file, rc = %d", rc);

    // create page header structure and we are going to copy to each page
    strncpy(header.eyeCatcher_, DMS_PAGE_EYECATCHER.c_str(), DMS_PAGE_EYECATCHER_LEN);
    header.size_ = DMS_PAGESIZE;
    header.flag_ = DMS_PAGE_FLAG_NORMAL;
    header.numSlots_ = 0;
    header.slotOffset_ = sizeof(dmsPageHeader);
    header.freeSpace_ = DMS_PAGESIZE - sizeof(dmsPageHeader);
    header.freeOffset_ = DMS_PAGESIZE;
    
    // copy to other pages
    for (int i = 0; i < DMS_FILE_SEGMENT_SIZE; i += DMS_PAGESIZE) {
        memcpy(data + i, (char *)&header, sizeof(dmsPageHeader));
    }

    rwMutex_.wLock();
    freeMapSize = freeSpaceMap_.size();

    // insert into free space map
    for (int i = 0; i < DMS_PAGES_PER_SEGMENT; i++) {
        freeSpaceMap_.insert({ header.freeSpace_, i + freeMapSize });
    }

    // push to segment body
    body_.push_back(data);
    header_->size_ += DMS_PAGES_PER_SEGMENT;
    rwMutex_.wUnlock();

done:
    return rc;
error:
    goto done;
}

int dmsFile::initNew()
{
    int rc = EDB_OK;
    rc = extendFile(DMS_FILE_HEADER_SIZE);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to extend file, rc = %d", rc);
    rc = map(0, DMS_FILE_HEADER_SIZE, (void **)&header_);
    PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to map header, rc = %d", rc);

    strncpy(header_->eyeCatcher_, DMS_HEADER_EYECATCHER.c_str(), DMS_HEADER_EYECATCHER_LEN);
    header_->size_ = 0;
    header_->flag_ = DMS_HEADER_FLAG_NORMAL;
    header_->version_ = DMS_HEADER_VERSION_CURRENT;
done:
    return rc;
error:
    goto done;
}


int dmsFile::findPage(size_t requiredSize)
{
    auto iter = freeSpaceMap_.upper_bound(requiredSize);
    if (iter != freeSpaceMap_.end()) {
        return iter->second;
    }
    return INVALID_PAGEID;
}

int dmsFile::extendFile(int extendSize)
{
    int rc = EDB_OK;
    char temp[DMS_EXTEND_SIZE] = {0};

    memset(temp, 0, DMS_EXTEND_SIZE);
    if (extendSize % DMS_EXTEND_SIZE != 0) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Invalid extend size, must be multiple of %d", DMS_EXTEND_SIZE);
        goto error;
    }

    for (int i = 0; i < extendSize; i += DMS_EXTEND_SIZE) {
        fileOp_.seekToEnd();
        rc = fileOp_.write(temp, DMS_EXTEND_SIZE);
        PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to write file, rc = %d", rc);
    }

done:
    return rc;
error:
    goto done;
}


int dmsFile::searchSlot(char* page, RID& rid, slot_off_t& slotOff)
{
    int rc = EDB_OK;
    dmsPageHeader* pageHeader = NULL;

    if (nullptr == page) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "page is nullptr");
        goto error;
    }

    pageHeader = (dmsPageHeader *)page;
    if (pageHeader->numSlots_ < rid.sid_) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "slot is out of range, expected %d, max %d", rid.sid_, pageHeader->numSlots_);
        goto error;
    }
    
    slotOff = *(slot_off_t *)(page + sizeof(dmsPageHeader) + rid.sid_ * sizeof(slot_off_t));
done:
    return rc;
error:
    goto done;
}

int dmsFile::loadData()
{
    int rc = EDB_OK;
    int numPages = 0;
    int numSegments = 0;
    unsigned int numRecords = 0;
    char *data = nullptr;
    dmsPageHeader* pageHeader = nullptr;
    RID rid;
    slot_off_t offset;
    json record;

    if (nullptr == header_) {
        rc = map(0, DMS_FILE_HEADER_SIZE, (void **)&header_);
        PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to map file header, rc = %d", rc);
    }

    numPages = header_->size_;
    if (numPages % DMS_PAGES_PER_SEGMENT != 0) {
        rc = EDB_SYS;
        PD_LOG(PdLevel::ERROR, "Failed to load data, the size of partial segment is invalid");
        goto error;
    }

    numSegments = numPages / DMS_PAGES_PER_SEGMENT;
    for (int i = 0; i < numSegments; i++) {
        // map the segment
        rc = map(DMS_FILE_HEADER_SIZE + i * DMS_FILE_SEGMENT_SIZE, 
                DMS_FILE_SEGMENT_SIZE, 
                (void **)&data);
        PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to map semgent index %d, rc = %d", i, rc);
        
        body_.push_back(data);
        // initialize freeSpaceMap_ and initialize page haeder
        for (int j = 0; j < DMS_PAGES_PER_SEGMENT; j ++) {
            pageHeader = reinterpret_cast<dmsPageHeader *>(data + j * DMS_PAGESIZE);
            freeSpaceMap_.insert({ pageHeader->freeSpace_, (i + 1) * j });

            // create index
            numRecords = pageHeader->numSlots_;
            rid.pid_ = j;

            for (unsigned int s = 0; s < numRecords; s ++) {
                offset = *(slot_off_t *) (data + j * DMS_PAGESIZE +
                                            sizeof(dmsPageHeader) + s * sizeof(slot_off_t));
                if (DMS_SLOT_EMPTY == offset) {
                    continue;
                }

                record = json::parse(std::string(data + j * DMS_PAGESIZE +
                                    offset + sizeof(dmsRecord)));
                rid.sid_ = s;
                rc = indexManager_->isIdExist(record);
                PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to check idx, it had been exist when reload data, rc = %d", rc);
                rc = indexManager_->createIndex(record, rid);
                PD_RC_CHECK(rc, PdLevel::ERROR, "Failed to create index, rc = %d", rc);
            }   
        }
    }
done:
    return rc;
error:
    goto done;
}


void dmsFile::reArrangeSpace(char* page)
{
    char* start = page;
    char* end = page + DMS_PAGESIZE;
    int offset = 0;
    dmsPageHeader* pageHeader = nullptr;
    dmsRecord* record = nullptr;
    bool isRemoved = false;

    // move from page start to slot start
    start = start + sizeof(dmsPageHeader);

    pageHeader = reinterpret_cast<dmsPageHeader *>(page);
    // check every slot and rearrange the space if the slot is empty
    for (size_t i = 0; i < pageHeader->numSlots_; i++) {
        offset = *reinterpret_cast<int *>(page + i * sizeof(slot_off_t));
        if (DMS_SLOT_EMPTY != offset) {
            // move the record if exists the empty slot before
            record = reinterpret_cast<dmsRecord *>(page + offset);
            end = end - record->size_;
            if (isRemoved) {
                memmove(end, page + offset, record->size_);
                *reinterpret_cast<slot_off_t *>(page + i * sizeof(slot_off_t)) = end - page;
            }

        } else {
            isRemoved = true;
        }
    }
}
}
