
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
std::string url = "/s?q=qq";
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
    req.setHeader("User-Agent", "Mozilla/5.0 (Linux; Android 4.1.1; Nexus 7 Build/JRO03D) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166  Safari/535.19");

    client.send(req.toString());

    foo.wait();
    while (1) {
        sleep(1);
    }
    return 0;
}

// int main() 
// {
//      HttpRequest req;
//      req.setUrl(url);
//      req.setVersion(version);
//      req.setHeader("host", host);
//      req.setHeader("Accept-Encoding", ae);
//      req.setHeader("Connection", "close");
//      req.setHeader("User-Agent", "Mozilla/5.0 (Linux; Android 4.1.1; Nexus 7 Build/JRO03D) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166  Safari/535.19");
// 
//      std::string requestStr = req.toString();
//      InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
//      Socket socket(true);
//      socket.connect(addr);
// 
//      socket.send(requestStr.c_str(), requestStr.size());
// 
//      char buf[1024];
//      size_t count = socket.recv(buf, 1024);
//      while (count != 0) {
//          std::cout << std::string(buf, count) << std::endl;
//          count = socket.recv(buf, 1024);
//      }
//      socket.close();
//      return 0;
// }

