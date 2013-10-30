
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include "netlib.h"

using namespace dyc;
using namespace boost;


typedef boost::shared_ptr<SendTask> SendTaskPtr;

SendTaskPtr pingpong(RecvTask* task) {
    Head head = task->_head;
    char* mesg = task->_data;
    NOTICE("====get message====");
    NOTICE("len: %lu   type: %d", head._len, head._type);
    NOTICE("message: %s", mesg);
    SendTaskPtr sendtask( NEW SendTask(head, mesg) );
    return sendtask;
}

void* serverRun(void* p) {
    Server* server = (Server*) p;
    server->start();
    return NULL;
}

int main(int argc, char** argv) {
    int port = 8715;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    InetAddress addr("127.0.0.1", port);
    Server client(addr);
    client.setReadCallback(bind(&pingpong, _1));

    pthread_t pid;
    pthread_create(&pid, NULL, serverRun, &client);

    sleep(1);
    InetAddress saddr("127.0.0.1", 8714);
    Connection* conn = client.connect(saddr);
    const char* mesg = "pingpong";
    Head head(strlen(mesg), 19871114);

    SendTaskPtr task(NEW SendTask(head, mesg));
    conn->rawSend(task);

    getchar();
    client.stop();
    pthread_join(pid, NULL);
    dumpUnfreed();
    return 0;
}
