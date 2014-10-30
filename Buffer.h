
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
    Buffer(const char* data, size_t size) 
        : mData(const_cast<char*>(data)), 
          mReadPos(0), 
          mSize(size), 
          mFinish(false), 
          mFreeBuf(false) {
          mWritePos = size;
    }

    Buffer(size_t size=8*1024) 
        : mData(NULL), 
          mReadPos(0), 
          mSize(size), 
          mFinish(false), 
          mFreeBuf(true){
        mData = new char[size];
        mWritePos = 0;
    }

    ~Buffer() {
        if (mFreeBuf)
            DELETE(mData);
    }

    int64_t getMesgId(int64_t mesgId) { return mMesgId; }
    void setMesgId(int64_t mesgId) { mMesgId = mesgId; }
    void hasWriten(size_t len) { mWritePos += len; }

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

    int expand(size_t len) {
        char* tmpBuf = NEW char[readableSize() + len * 2];
        mSize = readableSize() + len * 2;
        std::copy(mData, mData+mWritePos, tmpBuf);
        DELETES(mData);
        mData = tmpBuf;
    }

    int append(const char* data, size_t len) {
        if (mWritePos + len > mSize) {
            if (expand(len) < 0) {
                return -1;
            }
        }
        std::copy(data, data+len, beginWrite());
        hasWriten(len);
        return 0;
    }

    size_t readableSize() const { return mWritePos - mReadPos; }
    size_t writableSize() const { return mSize - mWritePos; }

    char* beginWrite() { return mData + mWritePos; }
    const char* beginRead() { return mData + mReadPos; }
    const char* data() { return mData; }
    void setFinish() { mFinish = true; }
    bool isFinish() { return mFinish; }

    void reset() {
        mReadPos = 0;
        mWritePos = 0;
    }

private:
    char* mData;
    size_t mReadPos;
    size_t mWritePos;
    size_t mSize;
    bool mFinish;
    bool mFreeBuf;
    int64_t mMesgId;
    static const char kCRLF[];
};


#endif  // MUDUO_NET_BUFFER_H
