
export CPLUS_INCLUDE_PATH=../misc/common
export CC=g++
#export CFLAGS= -g -pipe -Wall -Werror -fPIC -I./include/ -I../common -L../common
export CFLAGS= -g -pipe -Wall -fPIC -I./include/ -I../misc/common -L../misc/common
VPATH=src

.PHONY:all
all: server client

server:Connection.o Client.o Server.o EventLoop.o Epoller.o InetAddress.o Socket.o server.cpp Task.o 
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

client:Connection.o Client.o Server.o EventLoop.o Epoller.o InetAddress.o Socket.o client.cpp Task.o 
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

Task.o:Task.cpp
	$(CC) $(CFLAGS) -c $< -o $@

Server.o:Server.cpp
	$(CC) $(CFLAGS) -c $< -o $@

Connection.o:Connection.cpp
	$(CC) $(CFLAGS) -c $< -o $@

Client.o:Client.cpp
	$(CC) $(CFLAGS) -c $< -o $@

InetAddress.o:InetAddress.cpp
	$(CC) $(CFLAGS) -c $< -o $@

EventLoop.o:EventLoop.cpp
	$(CC) $(CFLAGS) -c $< -o $@

Epoller.o:Epoller.cpp
	$(CC) $(CFLAGS) -c $< -o $@

#Connection.o:Connection.cpp
#	$(CC) $(CFLAGS) -c $< -o $@

#Listener.o:Listener.cpp
#	$(CC) $(CFLAGS) -c $< -o $@

#Server.o:Server.cpp
#	$(CC) $(CFLAGS) -c $< -o $@

Socket.o:Socket.cpp
	$(CC) $(CFLAGS) -c $< -o $@

ThreadPool.o:ThreadPool.cpp
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY:clean
clean:
	rm -rf src/*.o *.d *.o {file,}server {file,}client test

