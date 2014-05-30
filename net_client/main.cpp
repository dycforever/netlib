#include "util.h"
#include "Socket.h"
#include "Tokenizer.h"
#include "Mutex.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include "Condition.h"
#include <boost/bind.hpp>

using namespace dyc;
using namespace std;

typedef dyc::Socket* SocketPtr;
class HttpResponseParse {

public:
    HttpResponseParse():mPos(0), mCond(mLock){}

    int readData(SocketPtr socket) {
        char buf[1024];
        cout << "in read data" << endl;

        int count = socket->recv(buf, sizeof(buf));
        cout << "recv " << count << " bytes with errno: " << errno << " " << strerror(errno) << endl;

        if (count == 0) {
            mCond.notify();
            return 0;
        }

        if (count > 0) {
            mResponse.append(buf, count);
            findStr(mResponse, ("\r\n\r\n"));
        }

        return count;
    }

    int conn(SocketPtr socket) {
        cout << "connect success" << endl;
    }

    int writeData(SocketPtr socket) {
        std::string reqLine ="GET /gz2?gz2=false HTTP/1.1\r\nHost: localhost\r\n\r\n";
        int count = socket->send(reqLine.c_str(), reqLine.size()-mPos);
        mPos += count;
        cout << "send " << count << " bytes" << endl;
        return count;
    }

    std::string out() {
        return mResponse;
    }

    void wait() {
        cout << "will wait" << endl;
        mCond.wait();
        cout << "awake" << endl;
    }

private:
    std::string mResponse;
    int mPos;
    dyc::MutexLock mLock;
    dyc::Condition mCond;
};

void* thr_fn(void* data) {
    EventLoop* p = (EventLoop*)(data);
    p->loop();
}

int main() {
    Socket sock(false);

    InetAddress addr("127.0.0.1", 8714);

    boost::shared_ptr<Epoller> epoller = boost::shared_ptr<Epoller>(NEW Epoller());
    epoller->createEpoll();
    EventLoop* p = NEW EventLoop(epoller);
    boost::shared_ptr<EventLoop> loop = boost::shared_ptr<EventLoop>(p);

    Connection conn(&sock, loop);
    epoller->addRW(&conn);

    sock.connect(addr);
    HttpResponseParse parser;

    boost::function< int (SocketPtr) > readfunc = boost::bind(&HttpResponseParse::readData, &parser, _1);
    conn.setReadCallback(readfunc);

    boost::function< int (SocketPtr) > connfunc = boost::bind(&HttpResponseParse::readData, &parser, _1);
    conn.setConnCallback(connfunc);

    boost::function< int (SocketPtr) > writefunc = boost::bind(&HttpResponseParse::writeData, &parser, _1);
    conn.setWriteCallback(writefunc);

    pthread_t ntid;
    pthread_create(&ntid, NULL, thr_fn, p);

    parser.wait();

    cout << "output: " << parser.out() << endl;
    return 0;
}


//    sock.connect(addr);
//    std::string reqLine ="GET /gz2 HTTP/1.1\r\nHost: localhost\r\n\r\n";
//    sock.send(reqLine.c_str(), reqLine.size());
//    char buf[1024];
//    memset(buf, 0, 1024);
//    sock.recv(buf, 1024);
//
//    printf("output: =========\n%s\n==========\n", buf);
