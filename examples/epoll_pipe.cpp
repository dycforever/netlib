
#include <sys/unistd.h>
#include <sys/wait.h>

#include <vector>
#include <algorithm>
#include <numeric>
#include <pthread.h>

#include "common.h"
#include "netlib.h"


using namespace std;
using namespace dyc;

#define TEST_TIMES 1000
#define MEMSIZE MB

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

   	for (int i=0; i<TEST_TIMES; i++) {
		Timer timer;
		ret = poller.poll(&event);
		CHECK_ERRORNO((void*)-1, ret==1, "poll() failed");
		long long data = -1;
		if (event.events | EPOLLIN) {
			ssize_t ret = read(fd, &data, sizeof(data));
		    CHECK_ERRORNO((void*)-1, ret==sizeof(data), "read() %d failed", fd);
//			DEBUG("epoll read %ld bytes :%lld", ret , data);
		}

        uint64_t pass = timer.ms_spend();
		timespend.push_back(pass);
        ++count;
	}
    int totaltime = accumulate(timespend.begin(), timespend.end(), 0);
    print_container("syscall time spend", timespend);
    cout << "syscall time sum:" << totaltime << endl 
        << "avg time: " << totaltime * 1.0 / count << endl;
    return NULL;
}

int main() {

	int pipes[2];
    int ret = pipe(pipes);
	CHECK_ERRORNO(-1, ret == 0, "pipe() failed");
//	pthread_t pid;
//	pthread_create(&pid, NULL, epollThread, &pipes[0]);

    for (int i=0; i<TEST_TIMES; i++) {
        //            NOTICE("please input:");
        long long data =1234;
        ssize_t ret = write(pipes[1], &data, sizeof(data));
        CHECK_ERRORNO(-1, ret==sizeof(data), "write() pipe[%d] failed ret:%ld", pipes[1], ret);
        //    		NOTICE("write %ld bytes i:%d", ret, i);
    }
    //        pthread_join(pid, NULL);
    epollThread(&pipes[0]);

    char* src = NEW char[MEMSIZE];
    char* dest = NEW char[MEMSIZE];
    memset(src, 0, MEMSIZE);
    memset(dest, 0, MEMSIZE);

	vector<uint64_t> timespend;
    for (int i=0; i<TEST_TIMES; i++) {
        Timer memtimer;
        memcpy(dest, src, MEMSIZE);

        uint64_t pass = memtimer.ms_spend();
		timespend.push_back(pass);
    }

    int totaltime = accumulate(timespend.begin(), timespend.end(), 0);
    print_container("memcpy time spend", timespend);
    cout << "memcpy time sum:" << totaltime << endl 
        << "avg time: " << totaltime * 1.0 / TEST_TIMES << endl;

}


