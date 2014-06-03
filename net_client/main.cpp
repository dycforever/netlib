#include "util.h"
#include "Socket.h"
#include "Tokenizer.h"
#include "Mutex.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include "Condition.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <boost/bind.hpp>

#include <fstream>

using namespace dyc;
using namespace std;

typedef dyc::Socket* SocketPtr;

class HttpResponseParser {
public:
    HttpResponseParser():mPos(0), mCond(mLock){}

    int parseRespLine(std::string line) {
        size_t vEnd = line.find(' ');
        if (vEnd != std::string::npos) {
            mResp.setVersion(line.substr(0, vEnd));
        }

        size_t codeStart = (vEnd + 1);
        if (vEnd != std::string::npos) {
            mResp.setState(line.substr(codeStart, 3));
        }

        size_t decsStart = codeStart + 4;
        if (vEnd != std::string::npos) {
            mResp.setDesc(line.substr(decsStart, line.size()-decsStart));
        }
    }


    int parseRespHeader(std::string line) {
        size_t comma = line.find(":");
        mResp.setHeader(trim(line.substr(0, comma)), 
                trim(line.substr(comma+1, line.size()-1-comma)));
    }

    int parse(const std::string& resp) {
        std::string token;
        size_t start = 0;
        start = getToken(resp, start, token, "\r\n");
        parseRespLine(token);

        while(1) {
            start = getToken(resp, start, token, "\r\n");
//            cout << "header line: " << tokens[i] << endl;
            if (token == "")
                break;
            parseRespHeader(token);
        }
        std::string body(resp.begin()+start, resp.end());
        mResp.setBody(body);
    }

    int readData(Buffer& buffer) {
//        mResponse.append(buffer.get(buffer.readableSize()));
        size_t size = buffer.readableSize();
        char* buf = buffer.get(size);
        std::string resp(buf, size);
        parse(resp);
        mCond.notify();
        return 0;
    }

    int conn() {
        cout << "connect success" << endl;
    }

    std::string out() {
        return mResp.toString();
    }

    void wait() {
        MutexLockGuard guard(mLock);
        mCond.wait();
    }

private:
    std::string mResponse;
    int mPos;
    dyc::MutexLock mLock;
    dyc::Condition mCond;
    HttpResponse mResp;
};

void* thr_fn(void* data) {
    EventLoop* p = (EventLoop*)(data);
    p->loop();
}

class Client {

    typedef boost::function< int (Buffer&) > ReadCallbackFunc;
    typedef boost::function< int () > ConnCallbackFunc;
    typedef boost::function< int () > WriteCallbackFunc;

public:
    Client() {
    }

    Connection* connect(const InetAddress& addr) {
        mEpoller = boost::shared_ptr<Epoller>(NEW Epoller());
        mEpoller->createEpoll();

        mLoop = boost::shared_ptr<EventLoop>(NEW EventLoop(mEpoller));
        mSock = NEW Socket(false);

        Connection* conn = NEW Connection(mSock, mLoop);
        mEpoller->addRW(conn);

        sleep(1);
        mSock->connect(addr);
        return conn;
    }

    void start () {
        pthread_t ntid;
        pthread_create(&ntid, NULL, thr_fn, mLoop.get());
    }

private:
    SocketPtr mSock;
    boost::shared_ptr<EventLoop> mLoop;
    boost::shared_ptr<Epoller> mEpoller;
};

int main(int argc, char** argv) {
    InetAddress addr("127.0.0.1", 8714);
    Client client;
    Connection* conn = client.connect(addr);

    HttpResponseParser parser;
    boost::function< int (Buffer&) > readfunc = boost::bind(&HttpResponseParser::readData, &parser, _1);
    boost::function< int () > connfunc = boost::bind(&HttpResponseParser::conn, &parser);

    conn->setReadCallback(readfunc);
    conn->setConnCallback(connfunc);
    client.start();

    HttpRequest req;
    if (argc > 1) {
        cout << "argc: " <<argc << endl;
        req.setUrl(argv[1]);
    }
    req.setHeader("host", "fedora");

    conn->send(req.toString());
    parser.wait();

    cout << "output response: " << parser.out() << endl;
    return 0;
}

//    sock.connect(addr);
//    std::string reqLine ="GET /gz2/?gz2=false HTTP/1.1\nHost: localhost\n\n";
//    sock.send(reqLine.c_str(), reqLine.size());
//    char buf[1024];
//    memset(buf, 0, 1024);
//    sock.recv(buf, 1024);
//    printf("output: =========\n%s\n==========\n", buf);
//    return 0;


