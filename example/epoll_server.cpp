#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <sys/epoll.h>
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <errno.h> 
#include <signal.h> 
#include <fcntl.h> 
#include <string> 
#include <algorithm> 

#define SERVER_PORT 8714
#define LENGTH_OF_LISTEN_QUEUE 20
#define BUFFER_SIZE 1024
#define MAXEVENTS 1

std::string resp;

enum RETCODE{
    CONTINUE,
    ADDOUT,
    DELETE
};

void setNonblocking(int fd) {
    int opts = fcntl(fd, F_GETFL);
    opts = opts | O_NONBLOCK;
    fcntl(fd, F_SETFL, opts);
}

bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
                   str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

class Connection 
{
public:
    Connection(int fd) : mFd(fd), mRespPos(0) {}
    RETCODE Handle(uint32_t event);
    int GetFd() { return mFd; }

public:
private:
    int mFd;
    std::string mReq;
    size_t mRespPos;
};

RETCODE Connection::Handle(uint32_t event) 
{
    if (event == EPOLLIN) {
        char buf[1024];
        int ret = read(mFd, buf, 1024);
        if (ret == 0) {
            return DELETE;
        } else if (ret == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                return CONTINUE;
            } else {
                printf("unknown read errno: %d\n", errno);
                return DELETE;
            }
        } else {
            mReq += std::string(buf, ret);
            if (has_suffix(mReq, "\r\n\r\n")) {
//                printf("req: %s\n", mReq.c_str());
                return ADDOUT;
            }
//            printf("req: %s\n", mReq.c_str());
            return CONTINUE;
        }
    } else if (event == EPOLLOUT) {
        if (mRespPos == resp.size()) {
            return DELETE;
        }
        while(1) {
            size_t size = std::min(1000ul, resp.size() - mRespPos);
            int ret = write(mFd, resp.c_str() + mRespPos, size);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EINTR) {
                    return CONTINUE;
                } else {
                    printf("unknown write errno: %d %s\n", errno, strerror(errno));
                    return DELETE;
                }
            } else {
                mRespPos += ret;
                if (ret < size) {
                    printf("send incomplete\n");
//                    return CONTINUE;
                }
                if (mRespPos == resp.size()) {
                    return DELETE;       
                }
            }
            usleep(100);
        }
    } else {
        printf("unknown event: %u\n", event);
        return DELETE;
    }
}

void run()
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
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt));
    setNonblocking(server_socket);

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

    struct epoll_event server_event;
    server_event.data.ptr = new Connection(server_socket);
    server_event.events = EPOLLIN;
    printf("epoll_ctl [%d]\n", epoll_ctl(efd, EPOLL_CTL_ADD, server_socket, &server_event));

    struct epoll_event events[MAXEVENTS];
    while (1) {
        fflush(stdout);
        int n = epoll_wait (efd, events, MAXEVENTS, -1);
//        printf("epoll_wait [%d] events\n", n);
        for (int i = 0; i < n; i++) {
            Connection* conn = (Connection*)(events[i].data.ptr);
            if (conn == NULL) {
                printf("invalid conn[%p]", conn);
                continue;
            }

            if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP)) {
                printf("epoll error\n");
                printf("epoll_del [%d]\n", epoll_ctl(efd, EPOLL_CTL_DEL, conn->GetFd(), NULL));
                close(conn->GetFd());
                delete conn;
                continue;
            }

            if (conn->GetFd() == server_socket) {
                struct sockaddr_in client_addr;
                socklen_t length = sizeof(client_addr);
                int new_socket = accept(server_socket,(struct sockaddr*)&client_addr,&length);
                if (new_socket < 0) {
                    printf("Server Accept Failed!\n");
                    break;
                }
                setNonblocking(new_socket);
                Connection* conn = new Connection(new_socket);
                struct epoll_event* event = new struct epoll_event;
                event->data.ptr = conn;
                event->events = EPOLLIN|EPOLLET;
//                event->events = EPOLLIN;
                int ret = epoll_ctl(efd, EPOLL_CTL_ADD, conn->GetFd(), event);
                if (ret != 0) {
                    printf("epoll_add read [%d] %d %s\n", ret, errno, strerror(errno));
                }
            } else {
                RETCODE retCode = conn->Handle(events[i].events);
                if (retCode == DELETE) {
                    int ret = epoll_ctl(efd, EPOLL_CTL_DEL, conn->GetFd(), NULL);
                    if (ret != 0) {
                        printf("epoll_del [%d]\n\n\n", epoll_ctl(efd, EPOLL_CTL_DEL, conn->GetFd(), NULL));
                    }
                    printf("\n\n\n");
                    close(conn->GetFd());
                    delete conn;
                } else if (retCode == ADDOUT) {
                    struct epoll_event* event = new struct epoll_event;
                    event->data.ptr = conn;
                    event->events = EPOLLOUT|EPOLLET;
//                    event->events = EPOLLOUT;
                    int ret = epoll_ctl(efd, EPOLL_CTL_MOD, conn->GetFd(), event);
                    if (ret != 0) {
                        printf("epoll_add write [%d] %d %s\n", ret, errno, strerror(errno));
                    }
                }
            }

        }
    } // while accept()


    close(server_socket);
}

void sig_handle(int sig) {
    if (sig == EPIPE) {
        printf("recv EPIPE\n");
    }
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, sig_handle);
    resp += "HTTP/1.1 200 OK\r\nServer: Tengine/1.4.6\r\n\r\n";
    for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 1000; j++) {
            resp += "abcabcabc\n";
        }
    }
    run();
}

