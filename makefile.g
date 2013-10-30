CC=g++
CFLAGS=-O1 -g -pipe -Wall -Werror -fPIC -I../cpp_prog/include/ -I../include/
%.d:%.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $<  | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g'  > $@; 

all:src/Connection.cpp src/InetAddress.cpp 
	
