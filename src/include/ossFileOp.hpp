#ifndef _OSSFILEOP_H
#define _OSSFILEOP_H

#include "import.hpp"


#define OSS_HANDLE		   int
#define OSS_F_GETLK        F_GETLK64
#define OSS_F_SETLK        F_SETLK64
#define OSS_F_SETLKW       F_SETLKW64

#define oss_struct_statfs  struct statfs64
#define oss_statfs         statfs64
#define oss_fstatfs        fstatfs64
#define oss_struct_statvfs struct statvfs64
#define oss_statvfs        statvfs64
#define oss_fstatvfs       fstatvfs64
#define oss_struct_stat    struct stat64
#define oss_struct_flock   struct flock64
#define oss_stat           stat64
#define oss_lstat          lstat64
#define oss_fstat          fstat64
#define oss_open           open64
#define oss_lseek          lseek64
#define oss_ftruncate      ftruncate64
#define oss_off_t          off64_t
#define oss_close          close
#define oss_access         access
#define oss_chmod          chmod
#define oss_read           read
#define oss_write          write

#define OSS_INVALID_HANDLE_FD_VALUE (-1)


#define OSS_PRIMITIVE_FILE_OP_FWRITE_BUF_SIZE (1 << 11)
#define OSS_PRIMITIVE_FILE_OP_READ_ONLY     (((unsigned int)1) << 1)
#define OSS_PRIMITIVE_FILE_OP_WRITE_ONLY    (((unsigned int)1) << 2)
#define OSS_PRIMITIVE_FILE_OP_OPEN_EXISTING (((unsigned int)1) << 3)
#define OSS_PRIMITIVE_FILE_OP_OPEN_CREAT   (((unsigned int)1) << 4)
#define OSS_PRIMITIVE_FILE_OP_OPEN_TRUNC    (((unsigned int)1) << 5)


namespace edb {
using offsetType = unsigned int;

class ossFileOp
{
public:
    using handleType = OSS_HANDLE;

private:
    handleType fd_;
    bool isStdout_{false};
    ossFileOp(const ossFileOp&) = delete;
    ossFileOp& operator=(const ossFileOp&) = delete;

protected:
    void setFileHandle(int fd);

public:
    ossFileOp();
    int open(const char* filePath, unsigned int options=OSS_PRIMITIVE_FILE_OP_OPEN_CREAT);
    void openStdout();
    void close();

    handleType getFd()
    {
        return fd_;
    }

    bool isValid();
    int read(const size_t size, void* const buf, size_t* const readSize);
    int write(const void* buf, size_t len=0);
    int fWrite(const char* fmt, ...);

    int offset(offsetType* const rtnSize) const;
    void seekToOffset(const offsetType off);
    void seekToEnd();
    int getFileSize(unsigned int* rtnFileSize);
};



}

#endif