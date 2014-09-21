
#include "Socket.h"
#include "InetAddress.h"

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
//    socket.setLinger(true, 0);
    socket.setNonblocking();
    std::cout << " connect: " << socket.connect(addr) << std::endl;
    while(!socket.checkConnected()){}

    InetAddress peerAddr;
    socket.getPeerAddr(peerAddr);
    std::cout << "peer addr: " << peerAddr.toIpPort() << std::endl;

    int ret = -1;
    size_t sendBytes = 0;
    while ((ret = socket.send("1", 1)) == 1) {
        ++sendBytes;
    }
    
    if (ret == 0) {
        std::cout << "connection close" << std::endl;
        return 0;
    } else if (ret < 0 && errno == EAGAIN) {
        std::cout << "send bytes: " << sendBytes << std::endl;
        getchar();
        std::cout << "close(): " << socket.close();
    } else {
        std::cout << "connection close with:" << strerror(errno) << std::endl;
    }

//    ret = socket.recv(buf, 4);
//    if (ret != 4) {
//        std::cout << "recv " << ret << "bytes" << std::endl;
//    }
//    buf[4] = 0;
//    std::cout << "recv content: " << buf << std::endl;

}