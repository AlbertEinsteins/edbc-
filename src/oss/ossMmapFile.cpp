#include "ossMmapFile.hpp"
#include "import.hpp"
#include "common.hpp"
#include "pd.hpp"


namespace edb
{


int ossMmapFile::open(const std::string& filename, uint options)
{
    int rc = EDB_OK;
    latch_.lock();
    rc = fileOp_.open(filename.c_str(), options);
    if (EDB_OK != rc) {
        PD_LOG(PdLevel::ERROR, "Failed to open file %s, rc = %d", filename.c_str(), rc);
        goto error;
    }

    isOpened_ = true;

done:
    latch_.unlock();
    return rc;
error:
    goto done;
}


void ossMmapFile::close()
{
    latch_.lock();
    // free space
    for (auto it = segments_.begin(); 
        it != segments_.end(); 
        it ++) {
        munmap((void *)(*it).ptr_, (*it).length_);
    }
    segments_.clear();
    if (isOpened_) {
        fileOp_.close();
        isOpened_ = false;
    }
    latch_.unlock();
}

int ossMmapFile::map(unsigned long long offset, unsigned int length, void **ppAddress)
{
    int rc = EDB_OK;
    ossMmapSegment seg(0, 0, 0);
    void *segAddr = NULL;
    unsigned int fSize = 0;
    if (length == 0) {
        goto done;
    }
    latch_.lock();

    rc = fileOp_.getFileSize(&fSize);
    if (rc) {
        PD_LOG(PdLevel::ERROR, "Failed to get file size, errno = %d", rc);
        goto error;
    }
    // check whether the param is valid
    if (offset + length > fSize) {
        PD_LOG(PdLevel::ERROR, "The offset plus length is %ld greater than file size %d", 
            offset + length, fSize);
        goto error;
    }

    segAddr = mmap(NULL, length, PROT_READ | PROT_WRITE, 
        MAP_SHARED, fileOp_.getFd(), offset);
    if (MAP_FAILED == segAddr) {
        PD_LOG(PdLevel::ERROR,
            "Failed to map offset %ld length %d, errno = %d", offset, length, errno);
        if (ENOMEM == errno) {
            rc = EDB_OOM;
        } else if (EACCES == errno) {
            rc = EDB_PERM;
        } else {
            rc = EDB_SYS;
        }
        goto error;
    }
    seg.ptr_ = segAddr;
    seg.offset_ = offset;
    seg.length_ = length;

    segments_.push_back(seg);
    if (ppAddress) {
        *ppAddress = segAddr;
    }
done:
    latch_.unlock();
    return rc;
error:
    goto done;
}



}