
#ifndef BUFFER_H
#define BUFFER_H

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string>
#include <string.h>

#include "common.h"

class Buffer {
public:

    Buffer(const char* data, size_t size, bool full=false) : mData(const_cast<char*>(data)), mReadPos(0), mSize(size) {
        if (full)
            mWritePos = size;
        else
            mWritePos = 0;
    }

    void hasWriten(size_t len) {
        mWritePos += len;
    }

    void makeSpace(size_t size) {
        mData = NEW char[size];
        mSize = size;
    }

    char* get(size_t len=1) {
        char* ret = (mData + mReadPos);
        mReadPos += len;
        return ret;
    }

    void append(const std::string& str) {
        append(str.c_str(), str.size());
    }

    int append(const char* data, size_t len) {
        if (mWritePos + len > mSize) {
            return -1;
        }
        std::copy(data, data+len, beginWrite());
        hasWriten(len);
        return 0;
    }

    size_t readableSize() const { return mWritePos - mReadPos; }
    size_t writableSize() const { return mSize - mWritePos; }

    char* beginWrite() { return mData + mWritePos; }
    const char* beginRead() { return mData + mReadPos; }

    const char* data() { 
        return mData; 
    }

private:
    char* mData;
    size_t mReadPos;
    size_t mWritePos;
    size_t mSize;

    static const char kCRLF[];
};


#endif  // MUDUO_NET_BUFFER_H
