
#include <boost/bind.hpp>

#include "Connection.h"
#include "EventLoop.h"
#include "util.h"

namespace dyc {

Connection::Connection(SocketPtr socket, boost::shared_ptr<EventLoop> loop): _readingTask(NULL), 
                                         _socket(socket), _loop(loop) {
    _socket->setReadCallback(boost::bind(&Connection::recvData, this, _socket));
    _socket->setWriteCallback(boost::bind(&Connection::sendData, this, _socket));
}

int Connection::recvData(SocketPtr socket) {
    int headret = 0;
    int bodyret = 0;
    if (_readingTask == NULL) {
        _readingTask = NEW RecvTask();
        _readingTask->setMallocCallback(_mallocCallback);
    }
    headret = _readingTask->readHead(socket);
    if(headret > 0) {
        if (_readingTask->over()) {
            goto recvtask_over;
        }
        return 0;
    } else if (headret < 0) {
        FATAL("read head failed");
        return -1;
    }

    DEBUG("headret=%d finish:%d", headret, _readingTask->headFinish());
    // headret == 0
    if (!_readingTask->headFinish() ) {
        // TODO close
        _loop->remove(socket);
        DELETE(socket);
        return 0;
    }

    bodyret = _readingTask->readBody(socket);
    DEBUG("read body %d bytes", bodyret);

    if (bodyret == 0 && !socket->isConnected()) {
        _loop->remove(socket);
        DELETE(socket);
    } else if(bodyret < 0) {
        FATAL("read body failed");
        return -1;
    }
    if (_readingTask->over()) {
recvtask_over:
        // TODO create RecvData !!
        SendTaskPtr task = _readCallback(_readingTask);
        pushWrite(task);
        socket->enableWrite();
        _loop->updateSocket(socket);
        _readingTask->clear();
    }
    return 0;
}

int Connection::rawSend(SendTaskPtr task) {
    if (!task) {
        return 0;
    }
    pushWrite(task);
    _socket->enableWrite();
    _loop->updateSocket(_socket);
    return 0;
}

int Connection::send(const char* data, int64_t size) {
    Head head(size, 0);
    SendTaskPtr task(NEW SendTask(head, data));
    pushWrite(task);
    _socket->enableWrite();
    _loop->updateSocket(_socket);
    return 0;
}

int Connection::pushWrite(SendTaskPtr task) {
    CHECK_WARNING(-1, task != NULL, "task is NULL");
    MutexLockGuard lock(_listMutex);
    _writeTasks.push_back(task);
    return 0;
}

Connection::SendTaskPtr Connection::getNextTask(std::list<SendTaskPtr>& list) {
    MutexLockGuard lock(_listMutex);
    if (list.empty()) {
        return SendTaskPtr();
    }
    CHECK_ERROR(SendTaskPtr(), !list.empty(), "list is empty");
    SendTaskPtr task = list.front();
    list.pop_front();
    return task;
}

void Connection::readTaskComplete() {
}

int Connection::sendData(SocketPtr socket) {
    if (!_writingTask) {
        _writingTask = getNextTask(_writeTasks);
        if (!_writingTask) {
            socket->disableWrite();
            _loop->updateSocket(socket);
            return 0;
        }
    }
    int hasWrite = _writingTask->sendHead(socket);
    CHECK_ERRORNO(-1, hasWrite >= 0, "write socket[%d] failed, ret: %d", socket->fd(), hasWrite);

    if (_writingTask->getHeadLength() == 0) {
        goto sendtask_over;
    }

    hasWrite = _writingTask->sendBody(socket);
    CHECK_ERRORNO(-1, hasWrite >= 0, "write socket[%d] failed, ret: %d", socket->fd(), hasWrite);

    if (_writingTask->over()) {
sendtask_over:
        if (_writeCallback) {
            _writeCallback(_writingTask);
        }
        _writingTask = getNextTask(_writeTasks);
    }
    return 0;
}

}
