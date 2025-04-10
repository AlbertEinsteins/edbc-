#ifndef OSSMMAPFILE_HPP
#define OSSMMAPFILE_HPP

#include "ossFileOp.hpp"
#include "import.hpp"
#include "ossLatch.hpp"

namespace edb
{

class ossMmapFile
{
protected:
    using uint = unsigned int;
    using ulong = unsigned long;

    class ossMmapSegment
    {
    public:
        void* ptr_;
        uint length_;
        ulong offset_; 

        ossMmapSegment(void* ptr, uint length, ulong offset)
        : ptr_(ptr), length_(length), offset_(offset) {}

    };

    ossFileOp fileOp_;
    ossWLatch latch_;
    bool isOpened_;
    std::vector<ossMmapSegment> segments_;
    std::string filename_;


public:
    using const_iter =  std::vector<ossMmapSegment>::const_iterator;

    inline const_iter begin()
    {
        return segments_.begin();
    }

    inline const_iter end() 
    {
        return segments_.end();
    }

    inline uint segmentSize()
    {
        return segments_.size();
    }

public:
    ossMmapFile() 
    {
        isOpened_ = false;
    }

    ~ossMmapFile()
    {
        close();
    }

    int open(const std::string& filename, uint options);

    void close();
    
    int map(unsigned long long offset, unsigned int len, void **ppaddress);
};





}


#endif