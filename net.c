
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <string>
#include <iostream>

int main() {
    struct sockaddr_in addr;
    std::string ip = "127.0.0.1";
    inet_aton(ip.c_str(), &addr.sin_addr);

    char buf[16];
    inet_ntop(AF_INET, &addr.sin_addr, buf, 16);
    std::cout << "buf: " << buf << std::endl;
}
