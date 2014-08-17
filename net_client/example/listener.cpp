
#include "Socket.h"
#include "InetAddress.h"

using namespace std;
using namespace dyc;

int main(int argc, char** argv) {
    int port = 8714;
    if (argc == 2) {
        port = atoi(argv[1]);
    }
    InetAddress addr("0.0.0.0", port);
    Socket listener(true);
    int ret = listener.bind(addr);
    cout << "bind ret: " << ret << endl;

    ret = listener.listen();
    cout << "listen ret: " << ret << endl;
    char buf[1024];

    InetAddress peerAddr;
    int fd = listener.accept(peerAddr);
    cout << "peerAddr: "  << peerAddr.toIpPort() << endl;
    Socket socket(fd);
    
    ret = socket.recv(buf, 4);
    if (ret != 4) {
        std::cout << "recv " << ret << "bytes" << std::endl;
    }
    buf[4] = 0;
    std::cout << "recv content: " << buf << std::endl;

    ret = socket.send("pong", 4);
    if (ret != 4) {
        std::cout << "send " << ret << "bytes" << std::endl;
    }

    std::cout << "listener sleep" << std::endl;
    while(1) {sleep(1);}

}
