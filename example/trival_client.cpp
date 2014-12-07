#include "Socket.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include "http_client/HttpRequest.h"
#include "http_client/HttpResponse.h"
#include "http_client/HttpResponseParser.h"
#include "Client.h"
#include "netutils/Tokenizer.h"

#include <boost/bind.hpp>
#include <string>
#include <fstream>

using namespace dyc;

// global options:
std::string version = "HTTP/1.1";
std::string ae = "gzip";
std::string url = "/s?q=qq&gz2=false";
std::string ip = "42.120.169.24";
std::string port = "80";
std::string host = "m.sp.sm.cn";

class Foo
{
public:
    Foo() 
        : mDone(false),
          mCond(mLock) {
    }
    int readData(Buffer* buffer, Buffer* outptuBuffer) {
        if (buffer->isFinish()) {
            LockGuard<MutexLock> guard(mLock);
            mDone = true;
            std::cerr << "will notify" << std::endl;
            mCond.notify();
        }
        size_t size = buffer->readableSize();
        std::cout << std::string(buffer->get(size), size) << std::endl;;
    }

    void wait() {
        mLock.lock();
        while (!mDone) {
            std::cerr << "will wait" << std::endl;
            mCond.wait();
            std::cerr << "wait done" << std::endl;
        }
        mLock.unlock();
    }

    bool mDone;
    MutexLock mLock; 
    Condition mCond;
};

int main(int argc, char** argv) {
    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
    Client client;
    int ret = client.connect(addr);
    assert(ret == 0);

    Foo foo;
    boost::function< int (Buffer*, Buffer*) > readfunc = boost::bind(&Foo::readData, &foo, _1, _2);

    client.setReadCallback(readfunc);
    client.start();

    HttpRequest req;
    req.setUrl(url);
    req.setVersion(version);
    req.setHeader("host", host);
    req.setHeader("Accept-Encoding", ae);
    req.setHeader("Connection", "close");
//    req.setHeader("User-Agent", "Mozilla/5.0 (Linux; Android 4.1.1; Nexus 7 Build/JRO03D) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166  Safari/535.19");

    client.send(req.toString());

    foo.wait();
//    while (1) {sleep(1);}

    INFO_LOG("main exit");

    return 0;
}


//
// *****************
//
// void epollDeal(Socket& socket) {
//     struct epoll_event ev,events[2];
//     ev.data.fd = socket.fd();
//     ev.events = EPOLLIN;
// 
//     int epfd = epoll_create(256);
//     epoll_ctl(epfd, EPOLL_CTL_ADD, socket.fd(), &ev);
//     for (;;) {
//         int nfds = epoll_wait(epfd, events, 2, 100);
//         INFO_LOG("epoll wait %d events", nfds);
//         if (nfds != 1) {
//             continue;
//         }
//         if (events[0].data.fd != socket.fd()) {
//             INFO_LOG("unknown fd %d", socket.fd());
//         }
//         char recvBuf[1024];
//         size_t recvBytes = socket.recv(recvBuf, 1024);
//         std::cout << std::string(recvBuf, recvBytes) << std::endl;
//         INFO_LOG("recv %lu bytes", recvBytes);
//         if (recvBytes == 0) {
//             break;
//         }
//     }
// }
// 
// int main() 
// {
//      InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
//      Socket socket(false);
//      int ret = socket.connect(addr);
//      assert(ret == 0);
//  
//      HttpRequest req;
//      req.setUrl(url);
//      req.setVersion(version);
//      req.setHeader("host", host);
//      req.setHeader("Accept-Encoding", ae);
//      req.setHeader("Connection", "close");
//  
//      std::string mesg = req.toString();
//      size_t mesgLength = mesg.length();
//      size_t sentBytes = 0;
//      while (sentBytes != mesgLength) {
//         long tmp = socket.send(mesg.c_str() + sentBytes, mesgLength - sentBytes);
//         if (tmp != -1) {
//             sentBytes += tmp;
//             std::cout << "sent " << tmp << " bytes" << std::endl;
//         }
//         usleep(1000);
//      }
// 
//      epollThread(socket);
//      return 0;
// }


