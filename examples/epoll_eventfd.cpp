
#include <sys/eventfd.h>
#include <sys/wait.h>

#include <vector>
#include <algorithm>
#include <numeric>
#include <pthread.h>

#include "common.h"
#include "netlib.h"


using namespace std;
using namespace dyc;

#define TEST_TIMES 10

void* epollThread(void* efd) {
    NOTICE("thread run %d", *((int*)efd));
	Epoller poller;
	poller.createEpoll();
	
	int ret = poller.addRead(NEW Socket( *((int*)efd) ));
	CHECK_ERROR((void*)-1, ret==0, "addRead() failed");

	Epoller::Event event;
	int fd = *((int*)efd);
	vector<uint64_t> timespend;
    int count = 0;

   	while (true) {
		Timer timer;
		ret = poller.poll(&event);
		CHECK_ERRORNO((void*)-1, ret==1, "poll() failed");
		long long data = -1;
		long long datasum = 0;
		if (event.events | EPOLLIN) {
			ssize_t ret = read(fd, &data, sizeof(data));
		    CHECK_ERRORNO((void*)-1, ret==sizeof(data), "read() %d failed", fd);
			DEBUG("epoll read %ld bytes :%lld", ret , data);
		}

        uint64_t pass = timer.ms_spend();
		timespend.push_back(pass);
        datasum += data;
        ++count;
        if (datasum == 1234 * TEST_TIMES) {
            break;
        }
	}
    int totaltime = accumulate(timespend.begin(), timespend.end(), 0);
    cout << "time sum:" << totaltime << endl 
        << "avg time: " << totaltime * 1.0 / count;
    return NULL;
}

int main() {

//	int efd = eventfd(0, EFD_NONBLOCK);
	int efd = eventfd(0, 0);
	CHECK_ERRORNO(-1, efd>0, "eventfd() failed");
	pthread_t pid;
	pthread_create(&pid, NULL, epollThread, &efd);
//    int child = fork();
//    if (child == 0) {
//        epollThread(&efd);
//    }
//    else {
    	for (int i=0; i<TEST_TIMES; i++) {
            NOTICE("please input:");
            long long data =1234;
            ssize_t ret = write(efd, &data, sizeof(data));
    		CHECK_ERRORNO(-1, ret==sizeof(data), "write() efd[%d] failed ret:%ld", efd, ret);
    		NOTICE("write %ld bytes i:%d", ret, i);
    	}
        pthread_join(pid, NULL);
//        ::wait(NULL);
//    }
}


