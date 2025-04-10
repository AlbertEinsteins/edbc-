#ifndef DMS_HPP
#define DMS_HPP

#include "ossLatch.hpp"
#include "ossMmapFile.hpp"
#include <nlohmann/json.hpp>
#include "common.hpp"
#include "dmsRID.hpp"
#include "ixm.hpp"

namespace edb {
// define some constant related to dms
using json = nlohmann::json;
using slot_off_t = int;


/*************Record Structure ******************* */

const static int DMS_EXTEND_SIZE = (1 << 16);
const static int DMS_PAGESIZE = 4 * (1 << 20);
const static int DMS_MAX_PAGENUMBER = 256 * 1024;

const static std::string DMS_KEY_FILEIDNAME = "_id";
extern const char* gKeyFieldName;

const static int DMS_RECORD_FLAG_NORMAL = 0;
const static int DMS_RECORD_FLAG_DROPPED = 1;

struct dmsRecord 
{
    unsigned int size_;
    unsigned int flag_;
    char data_[0];
};

// dms header
const static std::string DMS_HEADER_EYECATCHER = "DMSH";
const static int DMS_HEADER_EYECATCHER_LEN = 4;
const static int DMS_HEADER_FLAG_NORMAL = 0;
const static int DMS_HEADER_FLAG_DROPPED = 1;

const static int DMS_HEADER_VERSION_0 = 0;
__attribute__((unused)) static int DMS_HEADER_VERSION_CURRENT = DMS_HEADER_VERSION_0;

struct dmsHeader 
{
    char eyeCatcher_[DMS_HEADER_EYECATCHER_LEN];
    unsigned int size_; // total pages
    unsigned int flag_;
    unsigned int version_;
};


/* Page structure */
/**
*
*    ----------------------------------------------
*    | page_header | slot_list | freespace | data |
*
*/
const static std::string DMS_PAGE_EYECATCHER = "PAGH";
const static int DMS_PAGE_EYECATCHER_LEN = 4;
const static int DMS_PAGE_FLAG_NORMAL = 0;
const static int DMS_PAGE_FLAG_DROPPED = 1;
const static int DMS_SLOT_EMPTY = 0xFFFFFFFF;

struct dmsPageHeader
{
    char eyeCatcher_[DMS_PAGE_EYECATCHER_LEN];
    unsigned int size_;
    unsigned int flag_;
    unsigned int numSlots_;
    unsigned int slotOffset_;
    unsigned int freeSpace_;
    unsigned int freeOffset_;
    char         data_[0];
};

const static int DMS_MAX_RECORD = (DMS_PAGESIZE - sizeof(dmsHeader) - sizeof(dmsRecord) - sizeof(slot_off_t));



const static int DMS_FILE_SEGMENT_SIZE = 128 * (1 << 20); // 128m
const static int DMS_FILE_HEADER_SIZE = 64 * (1 << 10); // 64k
const static int DMS_PAGES_PER_SEGMENT = (DMS_FILE_SEGMENT_SIZE / DMS_PAGESIZE);
const static int DMS_MAX_SEGMENTS = (DMS_FILE_SEGMENT_SIZE / DMS_PAGES_PER_SEGMENT);


class dmsFile : public ossMmapFile
{
private:
    dmsHeader *header_;
    std::vector<char *> body_;

    std::multimap<unsigned int, page_id_t> freeSpaceMap_;
    ossRWLatch rwMutex_;    // global database lock
    ossWLatch extendMutex_;
    char* pFilename_;
    std::shared_ptr<ixmHashManager> indexManager_;

public:
    dmsFile(std::shared_ptr<ixmHashManager> indexMgr);
    ~dmsFile();

    // initial the dms file
    int initialize(const char *pFilename);

    // insert into file
    int insert(json& record, json& outRecord, RID& rid);
    int remove(RID& rid);
    int find(RID& rid, json& rtnRow);

private:
    // create a new segment for current file
    int extendSegment();
    // init from empty file, creating header only
    int initNew();
    // extend file for given bytes
    int extendFile(int extendSize);

    // load data from begining
    int loadData();

    // search slot
    int searchSlot(char* page, RID& rid, slot_off_t& slotOff);

    // reorganization
    void reArrangeSpace(char* page);

    // update free space
    void updateFreeSpace(dmsPageHeader* header, int changeSize, page_id_t pid);
    // find a page id to insert, return invalid_page_id if there's no page can be found for required size bytes;
    int findPage(size_t requiredSize);

public:
    inline unsigned int getNumSegments()
    {
        return body_.size();
    }

    inline unsigned int getNumPages()
    {
        return getNumSegments() * DMS_PAGES_PER_SEGMENT;
    }

    inline char* pageToOffset(page_id_t pid)
    {
        if (pid >= getNumPages()) {
            return NULL;
        }
        return body_[pid / DMS_PAGES_PER_SEGMENT] + DMS_PAGESIZE * (pid % DMS_PAGES_PER_SEGMENT);
    }

    inline bool validSize(size_t size)
    {
        if (size < DMS_FILE_HEADER_SIZE) {
            return false;
        }
        size = size - DMS_FILE_HEADER_SIZE;
        if (size % DMS_FILE_SEGMENT_SIZE != 0) {
            return false;
        }
        return true;
    }
};


}

#endif