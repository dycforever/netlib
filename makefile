
# configuration
COM_LIB_PATH=../misc/common

export CPLUS_INCLUDE_PATH=$(COM_LIB_PATH)
export CC=g++
export CFLAGS= -g -pipe -Wall -fPIC -I./include/ -I$(COM_LIB_PATH) -L$(COM_LIB_PATH)
VPATH=src

.PHONY:all
all: server client

libObj=Connection.o Client.o Server.o EventLoop.o Epoller.o InetAddress.o Socket.o Task.o

expObj=server.o client.o

bin=server client
$(bin):%:%.o $(libObj)
	g++ -o $@ $(CFLAGS) $^ -lpthread -lcommon

$(libObj):%.o:%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(expObj):%.o:%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -rf src/*.o *.d *.o {file,}server {file,}client test

