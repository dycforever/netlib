#include "util.h"
#include "Socket.h"
#include "Mutex.h"
#include "InetAddress.h"
#include "EventLoop.h"

#include <boost/bind.hpp>


typedef dyc::Socket* SocketPtr;
class HttpResponseParse {

public:
    HttpResponseParse():mPos(0){}

    int readData(SocketPtr socket) {
        char buf[1024];
        int count = socket->recv(buf, sizeof(buf));
        std::string str;
        str.append(buf, count);
        mLock.unlock();
    }

    int conn(SocketPtr socket) {
        std::string reqLine ="GET /gz2 HTTP/1.1\r\nHost: localhost\r\n\r\n";
        int count = socket->send(reqLine.c_str(), reqLine.size()-mPos);
        mPos += count;
    }

    std::string out() {
        return mResponse;
    }

    void wait() {
        mLock.lock();
    }

private:
    std::string mResponse;
    int mPos;
    dyc::MutexLock mLock;
};



int main() {
    using namespace dyc;
    using namespace std;
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
