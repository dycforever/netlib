
#include "Socket.h"
#include "InetAddress.h"
#include "Server.h"

using namespace dyc;

// int readData(Buffer* inputBuf, Buffer* outputBuf) {
//     size_t size = inputBuf->readableSize();
//     char* buf = inputBuf->get(size);
//     std::string str(buf, size);
//     std::cout << "received data:" << str << std::endl;
//     outputBuf->append("pong");
// }
// 
// int main() {
//     Server server(8714);
//     server.setReadCallback(readData);
//     server.start();
// }
    
int main(int argc, char** argv) {
    int port = 8714;
    if (argc == 2) {
        port = atoi(argv[1]);
    }
    InetAddress addr("0.0.0.0", port);
    Socket listener(true);
    int ret = listener.bind(addr);

    ret = listener.listen();
    char buf[1024];

    InetAddress peerAddr;
    int fd = listener.accept(peerAddr);
    std::cout << "peerAddr: "  << peerAddr.toIpPort() << std::endl;
    Socket socket(fd);
    
    std::cout << "listener sleep" << std::endl;

    while(1) {
        getchar();
        ret = socket.recv(buf, 1, NULL);
        std::cout << "recv: " << ret << std::endl;
    }

}


