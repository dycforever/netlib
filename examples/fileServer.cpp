#include <iostream>
#include <list>

#include <boost/bind.hpp>
#include "netlib.h"
#include "Task.h"

using namespace dyc;
using namespace std;
using namespace boost;

int showRead(Socket* socket) {
    int fd = socket->fd();
    char buf[1024];
    int count = read(fd, buf, 1024);
    if (count < 0) {
        FATAL("fd[%d] errno[%d]: %s", fd, errno, strerror(errno));
        return -1;
    }
    if (count == 0) {
        DEBUG("recv 0 byte now will close fd[%d]", fd);
        close(fd);
    }
    buf[count] = 0;
    DEBUG("recv %d bytes: %s", count, buf);
    return 0;
}

int echoWrite(Socket* socket) {
    int fd = socket->fd();
    char buf[] = "hello back";
    int count = write(fd, buf, sizeof(buf));
    if (count < 0) {
        FATAL("fd[%d] errno[%d]: %s", fd, errno, strerror(errno));
        return -1;
    }

    DEBUG("send back %lu bytes: %s", sizeof(buf), buf);
    return 0;
}

typedef boost::shared_ptr<SendTask> SendTaskPtr;
typedef boost::function< SendTaskPtr (RecvTask*) > ReadCallbackFunc;

int REQ=1;
int REP=2;

SendTaskPtr replyFileSize(RecvTask* task) {
    Head head = task->getHead();
    int type = head._type;
    SendTaskPtr sendtask;
    if (type == REQ) {
        char* path = task->getData();
        DEBUG("look for file [%s]", path);
        uint64_t len = 0;
        int ret = getFileSize(path, len);
        CHECK_ERROR(SendTaskPtr(), ret==0, "get file[%s] size failed", path);

        char* buf = NEW char[len];
        CHECK_ERROR(SendTaskPtr(), buf!=NULL, "new buffer size[%lu] failed", len);
        DEBUG("new mem [%p]", buf);

        FILE* fp = fopen(path, "r");
        ret = fread(buf, 1, len, fp);
        DEBUG("read %d bytes from file:%s", ret, path);
        fclose(fp);

        Head head(len, REP);
        sendtask.reset(NEW SendTask(head, buf) );
    } else if(type == REP) {
        char* mem = task->getData();
        uint64_t len = task->getHead()._len;
        DEBUG("received file size[%lu]", len);
        FILE* fp = fopen("/home/dyc/output", "w");
        int ret = fwrite(mem, 1, len, fp);
        DEBUG("write %d bytes to file", ret);
        fclose(fp);
        DEBUG("delete mem %p", mem);
        DELETES(mem);
    } else {
        FATAL("invalid type[%d]", type);
    }
    return sendtask;
}

char* newMem(const Head& head) {
    char* buf = NEW char[head._len];
    DEBUG("new mem[%p] in malloc callback size[%lu]", buf, head._len);
    return buf;
}

int freeMem(SendTaskPtr task) {
    const char* buf = task->getData();
    NOTICE("free mem %p", buf);
    DELETES(buf);
    return 0;
}


void* serverRun(void* p) {
    Server* server = (Server*) p;
    server->start();
    return NULL;
}

int main(int argc, char** argv) {
//    InetAddress addr("172.17.229.85", 8714);
    int port = 8714;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    InetAddress addr("192.168.1.75", port);
    Server server(addr);
    server.setReadCallback(bind(&replyFileSize, _1));
    server.setWriteCallback(bind(&freeMem, _1));
    server.setMallocCallback(bind(&newMem, _1));
    pthread_t pid;
    pthread_create(&pid, NULL, serverRun, &server);
    getchar();
    NOTICE("server will quit");
    dumpUnfreed();
    server.stop();
    pthread_join(pid, NULL);
}

