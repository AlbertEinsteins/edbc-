#include "common.hpp"
#include "import.hpp"
#include "ossFileOp.hpp"



namespace edb
{

ossFileOp::ossFileOp()
{
    fd_ = OSS_INVALID_HANDLE_FD_VALUE;
    isStdout_ = false;
}

bool ossFileOp::isValid()
{
    return (OSS_INVALID_HANDLE_FD_VALUE != fd_);
}

void ossFileOp::close()
{
    if (isValid() && !isStdout_ ) {
        ::oss_close(fd_);
        fd_ = OSS_INVALID_HANDLE_FD_VALUE;
    }

}

int ossFileOp::open(const char* filePath, unsigned int options)
{
    int rc = EDB_OK;
    int mode = O_RDWR;

    if (options & OSS_PRIMITIVE_FILE_OP_READ_ONLY) {
        mode = O_RDONLY;
    } else if (options & OSS_PRIMITIVE_FILE_OP_WRITE_ONLY) {
        mode = O_WRONLY;
    }

    if (options & OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING) {

    } else if (options & OSS_PRIMITIVE_FILE_OP_OPEN_CREAT) {
        mode |= O_CREAT;
    }
    
    if (options & OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC) {
        mode |= O_TRUNC;
    }

    do {
        fd_ = ::oss_open(filePath, mode, 0644);
    } while ((OSS_INVALID_HANDLE_FD_VALUE ==  fd_) && (EINTR == fd_));

    if (fd_ <= OSS_INVALID_HANDLE_FD_VALUE) {
        rc = errno;
    }
    
    return rc;
}

void ossFileOp::openStdout()
{
    fd_ = STDOUT_FILENO;
    isStdout_ = true;
}

void ossFileOp::seekToEnd()
{
    ::oss_lseek(fd_, 0, SEEK_END);
}

void ossFileOp::seekToOffset(offsetType off)
{
    oss_lseek(fd_, off, SEEK_SET);   
}

int ossFileOp::read(const size_t size, void* const buf, size_t* const readSizeActullay)
{
    int rc = EDB_OK;
    ssize_t readBytes = 0;

    if (!isValid()) {
        goto error;
    } 

    do {
        readBytes = ::oss_read(fd_, buf, size);
    } while ((-1 == readBytes) && (EINTR == readBytes));

    if (-1 == readBytes) {
        goto error;
    }

    *readSizeActullay = readBytes;

done:
    return rc;
error:
    *readSizeActullay = 0;
    rc = errno;
    goto done;
}

int ossFileOp::write(const void* buf, size_t len)
{
    int rc = 0;
    size_t writeBytes = 0;
    if (0 == len) {
        len = strlen((const char *)buf);
    }

    if (!isValid()) {
        return rc;
    }

    do {
        rc = ::oss_write(fd_, (const char *)buf + writeBytes, len - writeBytes);
        if (rc >= 0) {
            writeBytes += rc;
        }
    } while ((-1 == rc && EINTR == rc) || (-1 != rc && writeBytes < len));

    if (-1 == rc) {
        rc = errno;
    } else {
        rc = EDB_OK;
    }
    return rc;
}

int ossFileOp::fWrite(const char* fmt, ...)
{
    int rc = 0;
    va_list args;
    char buf[OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE] = {0};

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    rc = write(buf);
    return rc;
}

int ossFileOp::offset(offsetType* const rtnSize) const 
{
    int rc = EDB_OK;
    oss_struct_stat stat_val;

    if (-1 == ::oss_fstat(fd_, &stat_val)) {
        rc = errno;
        goto error;
    }
    *rtnSize = stat_val.st_size;
done:
    return rc;
error:
    *rtnSize = 0;
    goto done;
}

int ossFileOp::getFileSize(unsigned int* rtnFileSize)
{
    int rc = EDB_OK;
    oss_struct_stat stat = {};

    if (-1 == oss_fstat(fd_, &stat)) {
        rc = errno;
        goto error;
    }
    *rtnFileSize = stat.st_size;

done:
    return rc;
error:
    *rtnFileSize = 0;
    goto done;
}
}