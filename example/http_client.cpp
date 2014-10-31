
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

int parseArg(int argc, char** argv) {
    int c;
    while((c = getopt(argc, argv, "v:u:a:hp:i:H:")) != -1) {
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
            case 'H':
                host = optarg;
                break;
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'h':
                printf("usage: -v[0|1] -u url -a accept-encoding -H host -i ip -p port\n");
                exit(0);
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
    req.setHeader("User-Agent", "Mozilla/5.0 (Linux; Android 4.1.1; Nexus 7 Build/JRO03D) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166  Safari/535.19");

    client.send(req.toString());
    parser.wait();

    parser.getResponse().dump();
    std::cout << "end success" << std::endl;
    return 0;
}


