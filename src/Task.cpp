

#include "common.h"
#include "Socket.h"
#include "Task.h"

namespace dyc {

void printHead(Head* head) {
//    DEBUG("head: len[%lu]  [%d]", head->_len, head->_type);
}

int64_t SendTask::sendHead(SocketPtr socket) {
    if (_hpos == AFTER(&_head)) {
        return 0;
    }
    DEBUG("begin to write head");

    int count = socket->send(_hpos, _needWrite);
    CHECK_ERRORNO(-1, count >= 0, "socket[%d] read failed", socket->fd());

    _hpos += count;
    _needWrite -= count;

    if (_needWrite == 0) {
        // +1 for c-string
        _needWrite = _head._len;
        _bpos = _data;
        DEBUG("finish write head, head.len[%lu]", _head._len);
        printHead(&_head);
    }
    return count;
}

int64_t SendTask::sendBody(SocketPtr socket) {
    DEBUG("begin to write body");
        printHead(&_head);
    int count = 0;
    if (_bpos != _data + _head._len) {
        count = socket->send(_bpos, _needWrite);
        CHECK_ERRORNO(-1, count >= 0, "socket[%d] read failed", socket->fd());
        _bpos += count;
        _needWrite -= count;
        if (_needWrite == 0) {
            _finish = true;
        }
    }
    return count;
}

void SendTask::clear() {
    _hpos = (char*)&_head;
    _bpos = _data;
    _needWrite = sizeof(_head);
    _finish = false;
}

    //
    // RecvTask below
    //
RecvTask::RecvTask():_head(0ul, 0), _hpos((char*)&_head), _needRead(sizeof(_head)) {
    _data = NULL;
    _bpos = NULL;
    _finish = false;
}

int RecvTask::clear() {
    _hpos = ((char*)&_head);
    _needRead = sizeof(_head);
    DELETES(_data);
    _bpos = NULL;
    _finish = false;
    return 0;
}

int RecvTask::readHead(SocketPtr socket) {
    if (_hpos == AFTER(&_head)) {
        return 0;
    }

    int count = socket->recv(_hpos, _needRead);
    CHECK_ERRORNO(-1, count >= 0, "socket[%d] read failed", socket->fd());

    _hpos += count;
    _needRead -= count;

    if (_needRead == 0) {
        if (_mallocCallback) {
            _data = _mallocCallback(_head);
        } else {
        // +1 for c-string
            _data = NEW char[_head._len + 1];
        }
        _needRead = _head._len;
        _bpos = _data;
        DEBUG("finish read head, head.len[%lu]", _head._len);
        printHead(&_head);
        if (_needRead == 0) {
            _finish = true;
        }
    }
    return count;
}

int RecvTask::readBody (SocketPtr socket) {
    DEBUG("begin to read body");
    int count = 0;
    if (_bpos != _data + _head._len) {
        count = socket->recv(_bpos, _needRead);
        CHECK_ERRORNO(-1, count >= 0, "socket[%d] read failed", socket->fd());
        if (count == 0) {
            WARNING("no body whith head length: %lu", _head._len);
        }
        _bpos += count;
        _needRead -= count;
        if (_needRead == 0) {
//            *_bpos = 0;
            _finish = true;
        }
    }
    return count;
}

}


