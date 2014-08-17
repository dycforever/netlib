
#include "Socket.h"
#include "InetAddress.h"

using namespace std;
using namespace dyc;

int main(int argc, char** argv) {
    const char* addrStr = "127.0.0.1";
    int port = 8714;
    if (argc == 3) {
        addrStr = argv[1];
        port = atoi(argv[2]);
    }
    InetAddress addr(addrStr, port);
    Socket socket(true);
    int ret = socket.connect(addr);
    InetAddress peerAddr;
    socket.getPeerAddr(peerAddr);
    cout << ret << ": " << peerAddr.toIpPort() << endl;

    char buf[1024] = "ping";
    ret = socket.send(buf, 4);
    if (ret != 4) {
        std::cout << "send " << ret << "bytes" << std::endl;
    }

    ret = socket.recv(buf, 4);
    if (ret != 4) {
        std::cout << "recv " << ret << "bytes" << std::endl;
    }
    buf[4] = 0;
    std::cout << "recv content: " << buf << std::endl;

}