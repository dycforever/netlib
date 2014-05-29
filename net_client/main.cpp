#include "util.h"
#include "Socket.h"
#include "InetAddress.h"

int main() {
    int sockfd = dyc::createBlockingSocket();
    dyc::Socket sock(sockfd);
    const std::string addStr("127.0.0.1");
    dyc::InetAddress add(addStr, 8714);
    sock.connect(add);
    std::string reqLine ="GET /gz2 HTTP/1.1\r\nHost: localhost\r\n\r\n";
    sock.send(reqLine.c_str(), reqLine.size());
    char buf[1024];
    memset(buf, 0, 1024);
    sock.recv(buf, 1024);
    printf("output: =========\n%s\n==========\n", buf);
    return 0;
}
