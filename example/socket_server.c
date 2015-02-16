#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <sys/epoll.h>
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero

#define SERVER_PORT 8714
#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1024
#define MAXEVENTS 1

// both parent and child listen the same port
void all_listen()
{
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if( server_socket < 0) {
        printf("Create Socket Failed!");
        exit(1);
    }
    int opt =1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt));

    if( bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))) {
        printf("Server Bind Port : %d Failed!", SERVER_PORT); 
        exit(1);
    }

    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE) ) {
        printf("Server Listen Failed!"); 
        exit(1);
    }
    int pid = fork();
    if (pid > 0) {
        while (1) {
            struct sockaddr_in client_addr;
            socklen_t length = sizeof(client_addr);
            printf("parent will accept socket\n");
            int new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
            printf("parent accept socket fd[%d]\n", new_server_socket);
            if ( new_server_socket < 0) {
                printf("Server Accept Failed!\n");
                break;
            }
            sleep(2);
            printf("parent close[%d]\n", close(new_server_socket));
            sleep(10);
        } // while accept()
    } if (pid == 0) {
        while (1) {
            struct sockaddr_in client_addr;
            socklen_t length = sizeof(client_addr);
            printf("child will accept socket\n");
            int new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
            printf("child accept socket fd[%d]\n", new_server_socket);
            if ( new_server_socket < 0) {
                printf("Server Accept Failed!\n");
                break;
            }
            sleep(2);
            printf("child close[%d]\n", close(new_server_socket));
        } // while accept()
    } else {
        printf("fork failed\n");
    }

    close(server_socket);
}

// both parent and child use epoll_wait() to listen the same port
void all_listen_epoll()
{
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    int server_socket = socket(PF_INET,SOCK_STREAM,0);
    if( server_socket < 0) {
        printf("Create Socket Failed!");
        exit(1);
    }
    int opt =1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt));

    if( bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))) {
        printf("Server Bind Port : %d Failed!", SERVER_PORT); 
        exit(1);
    }

    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE) ) {
        printf("Server Listen Failed!"); 
        exit(1);
    }

    int efd = epoll_create(MAXEVENTS);
    if (efd == -1) {
        perror ("epoll_create");
        return;
    }

    struct epoll_event event;
    event.data.fd = server_socket;
    event.events = EPOLLIN;
    printf("epoll_ctl [%d]\n", epoll_ctl(efd, EPOLL_CTL_ADD, server_socket, &event));

    struct epoll_event events[MAXEVENTS];


    int pid = fork();
    if (pid > 0) {
        while (1) {
            printf("parent begin epoll_wait \n");
            int n = epoll_wait (efd, events, MAXEVENTS, -1);
            printf("parent epoll_wait [%d] events\n", n);
            if (n != 1 || events[0].data.fd != server_socket) {
                printf("should not be happened!\n");
                break;
            }
            if ((events[0].events & EPOLLERR) ||
                    (events[0].events & EPOLLHUP) ||
                    (!(events[0].events & EPOLLIN))) {
                fprintf (stderr, "epoll error\n");
                close (events[0].data.fd);
                continue;
            }

            struct sockaddr_in client_addr;
            socklen_t length = sizeof(client_addr);
            printf("parent will accept socket\n");
            int new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
            printf("parent accept socket fd[%d]\n", new_server_socket);
            if ( new_server_socket < 0) {
                printf("Server Accept Failed!\n");
                break;
            }
            printf("parent close[%d]\n", close(new_server_socket));
        } // while accept()
    } if (pid == 0) {
        while (1) {
            printf("child begin epoll_wait \n");
            int n = epoll_wait (efd, events, MAXEVENTS, -1);
            printf("child epoll_wait [%d] events\n", n);
            if (n != 1 || events[0].data.fd != server_socket) {
                printf("should not be happened!\n");
                break;
            }
            if ((events[0].events & EPOLLERR) ||
                    (events[0].events & EPOLLHUP) ||
                    (!(events[0].events & EPOLLIN))) {
                fprintf (stderr, "epoll error\n");
                close (events[0].data.fd);
                continue;
            }

            struct sockaddr_in client_addr;
            socklen_t length = sizeof(client_addr);
            printf("child will accept socket\n");
            int new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
            printf("child accept socket fd[%d]\n", new_server_socket);
            if ( new_server_socket < 0) {
                printf("Server Accept Failed!\n");
                break;
            }
            printf("child close[%d]\n", close(new_server_socket));
        } // while accept()
    } else {
        printf("fork failed\n");
    }

    close(server_socket);
}

void all_close()
{
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    int server_socket = socket(PF_INET,SOCK_STREAM,0);
    if( server_socket < 0) {
        printf("Create Socket Failed!");
        exit(1);
    }
    int opt =1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt));

    if( bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))) {
        printf("Server Bind Port : %d Failed!", SERVER_PORT); 
        exit(1);
    }

    if ( listen(server_socket, LENGTH_OF_LISTEN_QUEUE) ) {
        printf("Server Listen Failed!"); 
        exit(1);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        int new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
        printf("accept socket fd[%d]\n", new_server_socket);
        if ( new_server_socket < 0) {
            printf("Server Accept Failed!\n");
            break;
        }

        int pid = fork();
        if (pid > 0) {
            printf("parent close[%d]\n", close(new_server_socket));
            continue;
        } if (pid == 0) {
            char buffer[BUFFER_SIZE];
            bzero(buffer, BUFFER_SIZE);
            length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
            if (length < 0) {
                printf("Server Recieve Data Failed!\n");
                break;
            }
            printf("received [%s], length[%d]\n", buffer, length);

            if(send(new_server_socket, "pong", 4, 0)<0) {
                printf("Send File Failed\n");
                break;
            }
            printf("wait key press...\n");
            getchar();
            printf("child close[%d]\n", close(new_server_socket));
        } else {
            printf("fork failed\n");
            break;
        }

    } // while accept()

    close(server_socket);
}

void all_shutdown()
{
    struct sockaddr_in server_addr;
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);

    int server_socket = socket(PF_INET,SOCK_STREAM,0);
    if( server_socket < 0) {
        printf("Create Socket Failed!");
        exit(1);
    }
    int opt =1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt));

    if( bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))) {
        printf("Server Bind Port : %d Failed!", SERVER_PORT); 
        exit(1);
    }

    if ( listen(server_socket, LENGTH_OF_LISTEN_QUEUE) ) {
        printf("Server Listen Failed!"); 
        exit(1);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t length = sizeof(client_addr);
        int new_server_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
        printf("accept socket fd[%d]\n", new_server_socket);
        if ( new_server_socket < 0) {
            printf("Server Accept Failed!\n");
            break;
        }

        int pid = fork();
        if (pid > 0) {
            printf("parent shutdown[%d]\n", shutdown(new_server_socket, SHUT_WR));
            continue;
        } if (pid == 0) {
            char buffer[BUFFER_SIZE];
            bzero(buffer, BUFFER_SIZE);
            length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);
            if (length < 0) {
                printf("Server Recieve Data Failed!\n");
                break;
            }
            printf("received [%s], length[%d]\n", buffer, length);

            if(send(new_server_socket, "pong", 4, 0)<0) {
                printf("Send File Failed\n");
                break;
            }
            printf("wait key press...\n");
            getchar();
            printf("child shutdown[%d]\n", shutdown(new_server_socket, SHUT_WR));
        } else {
            printf("fork failed\n");
            break;
        }

    } // while accept()

    close(server_socket);
}


int main(int argc, char **argv)
{
    // only one accept epoll will be waken up
    // all_listen();
    
    // all epoll will be waken up
    // all_listen_epoll();

    // both close will cause disconnect
    // all_close();
    
    all_shutdown();
    return 0;
}


