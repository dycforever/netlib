
COM_LIB_PATH=/home/dyc/program/common/
NET_LIB_PATH=/home/dyc/netlib/

export CPLUS_INCLUDE_PATH=$(COM_LIB_PATH)
export CC=g++
export CFLAGS= -g -pipe -Wall -fPIC -I../include/ -I$(COM_LIB_PATH) -L$(COM_LIB_PATH) -L$(NET_LIB_PATH)
export LFLAGS=-lpthread -lcommon -lnet

.PHONY:all
all:myping
	
ping:myping.c ping_common.c ping_common.h
	gcc -O3 -fno-strict-aliasing -Wstrict-prototypes -Wall -g -D_GNU_SOURCE ping_common.c myping.c -o ping

myping:Ping.cpp
	g++ -o $@ $(CFLAGS) $^ $(LFLAGS)
#	g++ -O3 -fno-strict-aliasing -Wstrict-prototypes -Wall -g -D_GNU_SOURCE Ping.cpp -o Ping

.PHONY:clean
clean:
	rm -rf myping
