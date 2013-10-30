
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include "netlib.h"

using namespace dyc;
using namespace boost;

typedef boost::shared_ptr<SendTask> SendTaskPtr;

const int SEND = 1;
const int RECV = 2;
const int DATA = 3;

SendTaskPtr simpleMntl(RecvTask* task) {
    SendTaskPtr sendtask;
    Head head = task->_head;
    int type = head._type;
    if (type == SEND) {
        NOTICE("====get SEND message====");
        std::cout << "len: " << head._len << "   type: " << head._type << std::endl;       
    } else if(type == RECV) {
        NOTICE("====get RECV message====");
        std::cout << "len: " << head._len << "   type: " << head._type << std::endl;       
        head._type = DATA;
        const char* mesg = "hello world";
        sendtask.reset( NEW SendTask(head, mesg) );
    } else if(type == DATA) {
        NOTICE("====get DATA message====");
        std::cout << "len: " << head._len << "   mesg: " << task->_data<< std::endl;       
    } else {
        FATAL("fuck strange message type: %d", type);
    }
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
    client.setReadCallback(bind(&simpleMntl, _1));

    pthread_t pid;
    pthread_create(&pid, NULL, serverRun, &client);

    sleep(1);
    InetAddress saddr("127.0.0.1", 8714);
    Connection* conn = client.connect(saddr);
    const char* mesg = "a";
    Head head(strlen(mesg), RECV);

    SendTaskPtr task(NEW SendTask(head, mesg));
    conn->rawSend(task);

    getchar();
    client.stop();
    pthread_join(pid, NULL);
    dumpUnfreed();
    return 0;
}
