
#ifndef TASK_H
#define TASK_H

#include <boost/shared_ptr.hpp>
#include "Socket.h"

namespace dyc {

#define AFTER(p) ((char*)p)+sizeof(*p)

#pragma pack(push, 1)
    struct Head {
        uint64_t _len;
        int _type;
        int _srcRank;
        int _destRank;
        Head(uint64_t len, int type, int src=0, int dest=0):_len(len), _type(type), _srcRank(src), _destRank(dest) {}
    };
#pragma pack(pop)

class SendTask {
public:
    typedef Socket* SocketPtr;
    SendTask(Head head, const char* data):_head(head), _data(data) {
        clear();
    }

    int64_t sendHead(SocketPtr);
    int64_t sendBody(SocketPtr);
    bool over() {return _finish;}
    void clear();
    Head getHead() { return _head;}
    const char* getData() { return _data; }
    uint64_t getHeadLength() {return _head._len;}

private:
    Head _head;
    const char* _data;
    const char* _hpos;
    const char* _bpos;
    size_t _needWrite;
    bool _finish;
};

class Socket;

class RecvTask {
public:
    typedef Socket* SocketPtr;
    typedef boost::function<char* (const Head&)> MallocCallbackFunc;

    bool over() { return _finish; }
    bool headFinish() { return _hpos == AFTER(&_head); }
    RecvTask();
    ~RecvTask() {
        // TODO
//        DELETES(_data);
    }
    char* getData() { return _data; }
    Head getHead() { return _head;}
    int clear();

    int readHead(SocketPtr socket);
    int readBody (SocketPtr socket);

    void setMallocCallback(MallocCallbackFunc cb) { _mallocCallback = cb;}
    // TODO add class RecvMesg
//private:
    Head _head;
    char* _data;
    char* _hpos;
    char* _bpos;

    size_t _needRead;
    bool _finish;

    MallocCallbackFunc _mallocCallback;
    friend class Connection;
};

}

#endif
