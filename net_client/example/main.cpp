#include "util.h"
#include "Socket.h"
#include "Tokenizer.h"
#include "InetAddress.h"
#include "EventLoop.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpResponseParser.h"
#include "Client.h"

#include <boost/bind.hpp>

#include <fstream>

using namespace dyc;
using namespace std;

typedef dyc::Socket* SocketPtr;

// int main(int argc, char** argv) {
//#ifdef SHENMA
//    alog::Configurator::configureLogger("./logger.conf");
//#endif
//     std::string version = "HTTP/1.1";
//     std::string ae = "gzip";
//     std::string url = "/";
//     std::string ip = "127.0.0.1";
//     std::string port = "80";
//     std::string host = "localhost";
//     int c;
//     while((c = getopt(argc, argv, "v:u:a:h:p:i:")) != -1) {
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
//             case 'i':
//                 ip = optarg;
//                 break;
//             case 'p':
//                 port = optarg;
//                 break;
//             default:
//                 std::cerr << "parse options failed" << std::endl;
//                 return -1;
//         }
//     }
// 
//     InetAddress addr(ip, atoi(port.c_str()));    
//     Socket sock(true);
//     sock.connect(addr);
// 
//     HttpRequest req;
//     req.setUrl(url);
//     req.setVersion(version);
//     req.setHeader("host", host);
//     req.setHeader("Accept-Encoding", ae);
// 
//     string reqLine = req.toString();
//     sock.send(reqLine.c_str(), reqLine.size());
//     sock.setNonblocking();
//     char buf[1024];
//     memset(buf, 0, 1024);
//     std::string recvStr;
//     int recvCount = 0;
//     HttpResponseParser parser;
//     do {
//         recvCount = sock.recv(buf, 1024);
//         if (recvCount <= 0 && errno == EAGAIN) {
//             continue;
//         }
// 
//         if (recvCount <= 0) {
//             std::cout << " errno: " << errno << std::endl;
//             break;
//         }
// 
//         std::string tmp(buf, recvCount);
//         if (parser.parse(tmp) == DONE) {
//             break;
//         }
//     } while (1);
// 
//     parser.dump();
//     return 0;
// }


#ifdef SHENMA
alog::Logger* gLogger = alog::Logger::getLogger("httpc");
#endif

int main(int argc, char** argv) {
#ifdef SHENMA
    alog::Configurator::configureLogger("./logger.conf");
#endif
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
    Client client;
//    INFO("main connect");
    Connection* conn = client.connect(addr);
    assert(conn != NULL);

    HttpResponseParser parser;
    boost::function< int (Buffer&) > readfunc = boost::bind(&HttpResponseParser::readData, &parser, _1);
    boost::function< int () > connfunc = boost::bind(&HttpResponseParser::conn, &parser);

    conn->setReadCallback(readfunc);
    conn->setConnCallback(connfunc);
    client.start();

    HttpRequest req;
    req.setUrl(url);
    req.setVersion(version);
    req.setHeader("host", host);
    req.setHeader("Accept-Encoding", ae);

//    conn->send(req.toString());
    parser.wait();

//    cout << "output response: " << parser.out() << endl;
    parser.dump();
    return 0;
}

