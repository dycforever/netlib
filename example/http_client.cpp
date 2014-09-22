
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
    boost::function< int (Buffer*, Buffer*) > readfunc = boost::bind(&HttpResponseParser::readData, &parser, _1, _2);
    boost::function< int (bool) > connfunc = boost::bind(&HttpResponseParser::conn, &parser, _1);

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

    parser.getResponse().dump();
    return 0;
}

