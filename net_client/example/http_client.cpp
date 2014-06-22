
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

// global options:
std::string version = "HTTP/1.1";
std::string ae = "gzip";
std::string url = "/";
std::string ip = "127.0.0.1";
std::string port = "80";
std::string host = "localhost";

int parseArg(int argc, char** argv) {
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
    return 0;
} // parseArg()

int main(int argc, char** argv) {
    parseArg(argc, argv);

    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
    Client client;
    int ret = client.connect(addr);
    assert(ret == 0);

    HttpResponseParser parser;
    boost::function< int (Buffer&, Buffer&) > readfunc = boost::bind(&HttpResponseParser::readData, &parser, _1, _2);
    boost::function< int () > connfunc = boost::bind(&HttpResponseParser::conn, &parser);

    client.setReadCallback(readfunc);
    client.setConnCallback(connfunc);
    client.start();

    HttpRequest req;
    req.setUrl(url);
    req.setVersion(version);
    req.setHeader("host", host);
    req.setHeader("Accept-Encoding", ae);

    client.send(req.toString());
    parser.wait();

    parser.dump();
    return 0;
}


