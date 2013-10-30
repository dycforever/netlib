
export CPLUS_INCLUDE_PATH=../../common
export CC=g++
#export CFLAGS= -g -pipe -Wall -Werror -fPIC -I./include/ -I../common -L../common
export CFLAGS= -g -pipe -Wall -fPIC -I../include/ -I../../common -L../../common
export CXXFLAGS= -g -pipe -Wall -fPIC -I../include/ -I../../common -L../../common
VPATH=../src

allfiles=pingpongServer pingpongClient fileclient fileserver epoll_eventfd epoll_pipe
objfiles=Connection.o Client.o Server.o EventLoop.o Epoller.o InetAddress.o Socket.o Task.o 

.PHONY:all
all: $(allfiles)

epoll_eventfd: $(objfiles) epoll_eventfd.cpp
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

epoll_pipe: $(objfiles) epoll_pipe.cpp
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

pingpongServer:$(objfiles) pingpongServer.cpp 
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

pingpongClient: $(objfiles) pingpongClient.cpp
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

fileclient: $(objfiles) fileClient.cpp
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

fileserver: $(objfiles) fileServer.cpp
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

.PHONY:clean
clean:
	rm -rf ../src/*.o *.d *.o $(allfiles)
