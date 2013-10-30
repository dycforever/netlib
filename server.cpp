#include <iostream>
#include <list>

#include <boost/bind.hpp>
#include "netlib.h"
#include "Task.h"

using namespace dyc;
using namespace std;
using namespace boost;

typedef boost::shared_ptr<SendTask> SendTaskPtr;
typedef boost::function< SendTaskPtr (RecvTask*) > ReadCallbackFunc;

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
//    InetAddress addr("172.17.229.85", 8714);
    int port = 8714;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    InetAddress addr("127.0.0.1", port);
    Server server(addr);
//    server.setReadCallback(bind(&createSendTask, _1));
    server.setReadCallback(bind(&simpleMntl, _1));
    pthread_t pid;
    pthread_create(&pid, NULL, serverRun, &server);
    getchar();
    NOTICE("server will quit");
    server.stop();
    pthread_join(pid, NULL);
    dumpUnfreed();
}

