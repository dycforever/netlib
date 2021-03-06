
#include "Socket.h"
#include "InetAddress.h"

using namespace dyc;

std::string ip = "127.0.0.1";
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
}

int main(int argc, char** argv) {

    parseArg(argc, argv);
    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
    while(1) {
        Socket socket(true);
        socket.connect(addr); 
        InetAddress peerAddr;
        socket.getLocalAddr(peerAddr);
        INFO_LOG("client addr: %s", peerAddr.toIpPort().c_str());
    }
}

