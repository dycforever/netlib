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
public:
    HttpResponseParser():mResponse(""), mPos(0), mCond(mLock), mPhase(LINE)
{
    mTest = false;
}

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
        ret = mResp.setBody(mResponse);
        if (ret == DONE) {
            mPhase = LINE;
            return DONE;
        }
        return WAIT;
    }

    int readData(Buffer& buffer) {
        size_t size = buffer.readableSize();
        char* buf = buffer.get(size);
//        std::cout << "read " << size << " bytes data" << std::endl;
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

    void dump(const std::string& filename) {
        ofstream out(filename.c_str());
        out << "chunk: " << mResp.isChunked() << std::endl
                << "content-type: " << mResp.getContentEncoding() << std::endl
                << mResp.bodyToString() << std::endl;
        out.close();
    }

    void dump() {
        std::cout << "chunk: " << mResp.isChunked() << std::endl
                << "content-type: " << mResp.getContentEncoding() << std::endl
                << mResp.bodyToString() << std::endl;
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

    bool mTest;
    
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

int main(int argc, char** argv) {
    std::string version = "HTTP/1.1";
    std::string ae = "gzip";
    std::string url = "/";
    std::string ip = "127.0.0.1";
    std::string port = "80";
    std::string host = "localhost";
    int c;
    while((c = getopt(argc, argv, "v:u:a:h:p:i:")) != -1) {
        switch(c) {
            case 'v':
                if (std::string(optarg) == "0")
                    version = "HTTP/1.0";
                break;
            case 'u':
                url = optarg;
                break;
            case 'a':
                ae = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            default:
                std::cerr << "parse options failed" << std::endl;
                return -1;
        }
    }

    InetAddress addr(ip, atoi(port.c_str()));    
    Socket sock(true);
    sock.connect(addr);

    HttpRequest req;
    req.setUrl(url);
    req.setVersion(version);
    req.setHeader("host", host);
    req.setHeader("Accept-Encoding", ae);

    string reqLine = req.toString();
    sock.send(reqLine.c_str(), reqLine.size());
    sock.setNonblocking();
    char buf[1024];
    memset(buf, 0, 1024);
    std::string recvStr;
    int recvCount = 0;
    HttpResponseParser parser;
    do {
        recvCount = sock.recv(buf, 1024);
        if (recvCount <= 0 && errno == EAGAIN) {
            continue;
        }

        if (recvCount <= 0) {
            std::cout << " errno: " << errno << std::endl;
            break;
        }

        std::string tmp(buf, recvCount);
        if (parser.parse(tmp) == DONE) {
            break;
        }
    } while (1);

    parser.dump();
    return 0;
}



// int main(int argc, char** argv) {
//     std::string version = "HTTP/1.1";
//     std::string ae = "gzip";
//     std::string url = "/";
//     std::string host = "localhost";
//     int c;
//     while((c = getopt(argc, argv, "v:u:a:h")) != -1) {
//         switch(c) {
//             case 'v':
//                 if (std::string(optarg) == "0")
//                     version = "HTTP/1.0";
//                 break;
//             case 'u':
//                 url = optarg;
//                 break;
//             case 'a':
//                 ae = optarg;
//                 break;
//             case 'h':
//                 host = optarg;
//                 break;
//             default:
//                 std::cerr << "parse options failed" << std::endl;
//                 return -1;
//         }
//     }
// 
//     InetAddress addr("127.0.0.1", 8714);
//     Client client;
//     Connection* conn = client.connect(addr);
// 
//     HttpResponseParser parser;
//     boost::function< int (Buffer&) > readfunc = boost::bind(&HttpResponseParser::readData, &parser, _1);
//     boost::function< int () > connfunc = boost::bind(&HttpResponseParser::conn, &parser);
// 
//     conn->setReadCallback(readfunc);
//     conn->setConnCallback(connfunc);
//     client.start();
// 
//     HttpRequest req;
//     req.setUrl(url);
//     req.setVersion(version);
//     req.setHeader("host", host);
//     req.setHeader("Accept-Encoding", ae);
// 
//     conn->send(req.toString());
//     parser.wait();
// 
// //    cout << "output response: " << parser.out() << endl;
//     parser.dump();
//     return 0;
// }

