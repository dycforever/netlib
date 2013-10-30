#include <iostream>
#include "netlib.h"

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include "Task.h"


using namespace dyc;
using namespace boost;

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

    int port = 8715;

    InetAddress addr("127.0.0.1", port);
    Server client(addr);

    client.setReadCallback(bind(&replyFileSize, _1));
    client.setWriteCallback(bind(&freeMem, _1));
    client.setMallocCallback(bind(&newMem, _1));

    pthread_t pid;
    pthread_create(&pid, NULL, serverRun, &client);

    sleep(1);
    InetAddress saddr("127.0.0.1", 8714);
    Connection* conn = client.connect(saddr);
    const char* file = "/home/dyc/program/netlib/examples/makefile";
    Head head(strlen(file), REQ);

    SendTaskPtr task(NEW SendTask(head, file));
    conn->rawSend(task);

    getchar();
    client.stop();
    pthread_join(pid, NULL);

    dumpUnfreed();
    return 0;
}
