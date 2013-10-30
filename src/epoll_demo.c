#include "my_net_head.h"
#include "common.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <utils.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h> 

#include <iostream>
#include <fcntl.h>

int gcount=0;
extern void setup_server(int port,int &sfd,int &connfd,struct sockaddr_in &servaddr);

int main(){
	int sfd, connfd,fd;
	struct sockaddr_in servaddr;
	char buff[MAXLINE];
	time_t ticks;
	int count;
	setup_server(8714,sfd,connfd,servaddr);

	int sfd2;
	setup_server(8715,sfd2,connfd,servaddr);
    
    int epsfd = epoll_create(32);
    if( epsfd < 0 ){
        FATAL("create epoll failed errno:%d", errno);
        return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sfd;
    int ret = epoll_ctl(epsfd, EPOLL_CTL_ADD, sfd, &ev);
    if( ret < 0 ){
        FATAL("add sfd failed");
        return -1;
    }

    ev.events = EPOLLIN;
    ev.data.fd = sfd2;
    ret = epoll_ctl(epsfd, EPOLL_CTL_ADD, sfd2, &ev);
    if( ret < 0 ){
        FATAL("add sfd failed");
        return -1;
    }

    int client = 0;
    struct sockaddr local;
    socklen_t addrlen = sizeof(struct sockaddr);
    struct epoll_event *events;
    int maxevents = 5;
    events = NEW struct epoll_event[maxevents];
    if(events == NULL){
        FATAL("new events failed");
        return -1;
    }

    for(;;) {
        int nfds = epoll_wait(epsfd, events, maxevents, -1);
        for(int n = 0; n < nfds; ++n) {
            if(events[n].data.fd == sfd) {
                client = accept(sfd, (struct sockaddr *) &local,
                        &addrlen);
                if(client < 0){
                    WARNING("accept failed ret: %d", client);
                    perror("info");
                    continue;
                }
                NOTICE("accept 1 %d", sfd);
//                setnonblocking(client);
                ev.events = EPOLLOUT;
                ev.data.fd = client;
                if (epoll_ctl(epsfd, EPOLL_CTL_ADD, client, &ev) < 0) {
                    FATAL("epoll set insertion error: fd=%d0i",
                            client);
                    return -1;
                }
            }else if(events[n].data.fd == sfd2){
                client = accept(sfd2, (struct sockaddr *) &local,
                        &addrlen);
                if(client < 0){
                    WARNING("accept failed ret: %d", client);
                    perror("info");
                    continue;
                }
                NOTICE("accept 2 %d", sfd2);
//                setnonblocking(client);
                ev.events = EPOLLOUT;
                ev.data.fd = client;
                if (epoll_ctl(epsfd, EPOLL_CTL_ADD, client, &ev) < 0) {
                    FATAL("epoll set insertion error: fd=%d0i",
                            client);
                    return -1;
                }
            }else{
                connfd = events[n].data.fd;
                NOTICE("write conn %d", connfd);
                std::string resp = "hello I'm server\n";
			    written(connfd, resp.c_str(), resp.size());

                NOTICE("read from client:");
			    int read_cnt = readn(connfd, buff, sizeof(buff));
                NOTICE("%s", buff);

		        time_t ticks = time(NULL);
		        snprintf(buff,sizeof(buff),"%.24s\r\n",ctime(&ticks));
			    written(connfd, buff, strlen(buff));
			    close(connfd);
            } //else
        }
    }
	return 0;
}


