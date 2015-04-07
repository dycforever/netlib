
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "common.h"
#include "Socket.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "Server.h"

using namespace std;
using namespace dyc;

// global options:
std::string ip = "0.0.0.0";
std::string port = "8714";

int parseArg(int argc, char** argv) {
    int c;
    while((c = getopt(argc, argv, "hp:i:")) != -1) {
        switch(c) {
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'h':
            default:
                std::cout << "usage: " << argv[0] << " -i ip -p port" << std::endl;
                exit(0);
                return -1;
        }
    }
    return 0;
} // parseArg()


int main(int argc, char** argv)
{
    parseArg(argc, argv);
    Socket* mListenSocket = NEW Socket(true);
    assert(mListenSocket != NULL);

    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
    int ret = mListenSocket->bind(addr);
    assert(ret != -1);
    ret = mListenSocket->listen();
    assert(ret != -1);

    getchar();
    while(1) {
        InetAddress remoteAddr;
        mListenSocket->accept(remoteAddr);
        std::cout << "remoteAddr: " << remoteAddr.toIpPort() << std::endl;
    }
}

