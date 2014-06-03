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
private:
    enum ParsePhase {LINE, HEADER, BODY};
    enum ParseRet {DONE, WAIT};
public:
    HttpResponseParser():mResponse(""), mPos(0), mCond(mLock), mPhase(LINE){}

    ParseRet parseRespLine(std::string line) {
        size_t vEnd = line.find(' ');
        if (vEnd != std::string::npos) {
            mResp.setVersion(line.substr(0, vEnd));
        } else {
            return WAIT;
        }

        size_t codeStart = (vEnd + 1);
        if (vEnd != std::string::npos) {
            mResp.setState(line.substr(codeStart, 3));
        } else {
            return WAIT;
        }

        size_t decsStart = codeStart + 4;
        if (vEnd != std::string::npos) {
            mResp.setDesc(line.substr(decsStart, line.size()-decsStart));
        } else {
            return WAIT;
        }

        return DONE;
    }


    ParseRet parseRespHeader(std::string line) {
        size_t comma = line.find(":");
        if (comma != std::string::npos) {
            mResp.setHeader(trim(line.substr(0, comma)), 
                    trim(line.substr(comma+1, line.size()-1-comma)));
            return DONE;
        }
        return WAIT;
    }

    int parse(const std::string& resp) {
        std::string token;
        size_t start = 0;
        ParseRet ret;
        mResponse.append(resp);
//        cout << "at first:" << mResponse << endl;
        if (mPhase == LINE) {
            // TODO if too long, return error
            start = getToken(mResponse, start, token, "\r\n");
            ret = parseRespLine(token);
            if (ret == DONE) {
                mPhase = HEADER;
                mResponse = mResponse.substr(start, mResponse.size()-start);
                start = 0;
            } else {
                cout << "return WATI" << endl;
                return WAIT;
            }
        }

//        cout << "after parse header:" << mResponse << endl;
        while(mPhase == HEADER) {
            size_t ostart = start;
            start = getToken(mResponse, start, token, "\r\n");
//            cout << "header line: " << token << endl;
            if (token == "") {
                mResponse = mResponse.substr(start, mResponse.size()-start);
                mPhase = BODY;
                start = 0;
                break;
            }
            ret = parseRespHeader(token);
            if (ret != DONE) {
                mResponse = mResponse.substr(ostart, mResponse.size()-ostart);
                cout << "return WATI2" << endl;
                return WAIT;
            }
        }
        if (start != 0) {
            mResponse = mResponse.substr(start, mResponse.size()-start);
        }
//        cout << "body: " << body << endl;
        mResp.setBody(mResponse);
        return DONE;
    }

    int readData(Buffer& buffer) {
        size_t size = buffer.readableSize();
        char* buf = buffer.get(size);
        std::cout << "read " << size << " bytes data" << std::endl;
        std::string resp(buf, size);
        if (parse(resp) == DONE) {
            mCond.notify();
        }
        return 0;
    }

    int conn() {
    }

    std::string out() {
        return mResp.toString();
    }

    void wait() {
        LockGuard<MutexLock> guard(mLock);
        mCond.wait();
    }

private:
    std::string mResponse;
    int mPos;
    dyc::MutexLock mLock;
    dyc::Condition mCond;
    HttpResponse mResp;
    ParsePhase mPhase;
    
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
//        mEpoller = boost::shared_ptr<Epoller>(NEW Epoller());
        mEpoller = NEW Epoller();
        mEpoller->createEpoll();

//        mLoop = boost::shared_ptr<EventLoop>(NEW EventLoop(mEpoller));
        mLoop = NEW EventLoop(mEpoller);
        mSock = NEW Socket(false);

        Connection* conn = NEW Connection(mSock, mLoop);
        mEpoller->addRW(conn);

        mSock->connect(addr);
        return conn;
    }

    void start () {
        pthread_t ntid;
        pthread_create(&ntid, NULL, thr_fn, mLoop);
    }

private:
    SocketPtr mSock;
//    boost::shared_ptr<EventLoop> mLoop;
//    boost::shared_ptr<Epoller> mEpoller;
    EventLoop* mLoop;
    Epoller* mEpoller;
};

// int main(int argc, char** argv) {
//     InetAddress addr("127.0.0.1", 8714);
//     Socket sock(true);
//     sock.connect(addr);
// 
//     HttpRequest req;
//     if (argc > 1) {
//         req.setUrl(argv[1]);
//     }
//     req.setHeader("host", "localhost");
//     req.setVersion("HTTP/1.0");
// 
//     string reqLine = req.toString();
//     sock.send(reqLine.c_str(), reqLine.size());
// 
//     char buf[1024];
//     memset(buf, 0, 1024);
//     int recvCount = sock.recv(buf, 1024);
//     std::cout << "recv " << recvCount << " bytes" << std::endl;
//     std::string recvStr(buf, recvCount);
// 
//     HttpResponseParser parser;
//     parser.parse(recvStr);
//     cout << "parse success"<< endl;
// 
//    cout << "output response: " << parser.out() << endl;
//    return 0;
//}



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
        req.setUrl(argv[1]);
    }
    req.setHeader("host", "localhost");

    conn->send(req.toString());
    parser.wait();

    cout << "output response: " << parser.out() << endl;
    return 0;
}

