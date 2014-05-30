#include "util.h"
#include "Socket.h"
#include "Mutex.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include "Condition.h"
#include <boost/bind.hpp>

using namespace std;
typedef dyc::Socket* SocketPtr;
class HttpResponseParse {

public:
    HttpResponseParse():mPos(0), mCond(mLock){}

    int readData(SocketPtr socket) {
        char buf[1024];
        int count = socket->recv(buf, sizeof(buf));

        cout << "recv " << count << " bytes" << endl;

        mResponse.append(buf, count);
        mCond.notify();
    }

    int conn(SocketPtr socket) {
        std::string reqLine ="GET /gz2 HTTP/1.1\r\nHost: localhost\r\n\r\n";
        int count = socket->send(reqLine.c_str(), reqLine.size()-mPos);
        mPos += count;
        cout << "send " << count << " bytes" << endl;
    }

    std::string out() {
        return mResponse;
    }

    void wait() {
        cout << "will wait" << endl;
        mCond.wait();
        mCond.wait();
        cout << "awake" << endl;
    }

private:
    std::string mResponse;
    int mPos;
    dyc::MutexLock mLock;
    dyc::Condition mCond;
};



int main() {
    using namespace dyc;
    using namespace std;

    MutexLock lock;
    Condition cond(lock);
    cond.wait();
    cout << "hehe" << endl;

    pthread_cond_t pc;
    cout << pthread_cond_init(&pc, NULL) << endl;
    lock.lock();
    cout << pthread_cond_wait(&pc, lock.getPthreadMutex()) << endl;
    lock.unlock();
    cout << "hehe" << endl;


    Socket sock(true);

    InetAddress addr("127.0.0.1", 8714);

    boost::shared_ptr<Epoller> epoller = boost::shared_ptr<Epoller>(NEW Epoller());
    boost::shared_ptr<EventLoop> loop = boost::shared_ptr<EventLoop>(NEW EventLoop(epoller));

    Connection conn(&sock, loop);
    HttpResponseParse parser;

    boost::function< int (SocketPtr) > readfunc = boost::bind(&HttpResponseParse::readData, &parser, _1);
    conn.setReadCallback(readfunc);

    boost::function< int (SocketPtr) > connfunc = boost::bind(&HttpResponseParse::readData, &parser, _1);
    conn.setConnCallback(connfunc);

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
