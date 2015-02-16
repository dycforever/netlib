#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <iostream>

int main(int argc, char** argv)  
{  
    int send_sk;  
    struct sockaddr_in s_addr;  
    socklen_t len = sizeof(s_addr);  
    send_sk = socket(AF_INET, SOCK_STREAM, 0);  
    if(send_sk == -1)  
    {  
        perror("socket failed  ");  
        return -1;  
    }  
    bzero(&s_addr, sizeof(s_addr));  
    s_addr.sin_family = AF_INET;  

    inet_pton(AF_INET,"127.0.0.1" ,&s_addr.sin_addr);  
    s_addr.sin_port = htons(8714);  
    if(connect(send_sk,(struct sockaddr*)&s_addr,len) == -1)  
    {  
        return -1;  
    } 
    char pcContent[4096]={0};
    write(send_sk,pcContent,10);

    // for send RST
    // struct linger lingerVal = {1, 0};
    // setsockopt(send_sk, SOL_SOCKET, SO_LINGER, (char *) &lingerVal, sizeof(lingerVal));

    std::cout << "input [r] to shutdown READ" << std::endl;
    std::cout << "input [w] to shutdown WRITE" << std::endl;
    std::cout << "input [a] to shutdown READ and WRITE ??" << std::endl;
    char c = getchar();
    if (c == 'r') {
        std::cout << "shutdown RD" << std::endl;
        shutdown(send_sk, SHUT_RD);
    } else if (c == 'w') {
        std::cout << "shutdown WR" << std::endl;
        shutdown(send_sk, SHUT_WR);
    } else {
        std::cout << "shutdown RDWR" << std::endl;
        shutdown(send_sk, SHUT_RDWR);
    }
    // after shutdown RD, or SIGPIPE
    char buf[] = "try again!";
    int count = send(send_sk, buf, sizeof(buf), MSG_NOSIGNAL);
    std::cout << "write count: " << count << " buf: " << buf << std::endl;
    sleep(10);
} 

