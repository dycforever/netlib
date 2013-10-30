#ifndef NETLIB_LISTENER_H
#define NETLIB_LISTENER_H

#include <list>
#include <sys/socket.h>

#include "InetAddress.h"

namespace dyc {

class Socket;

class Client {
public:
    Client(const InetAddress& addr):_serverAddr(addr) {}
    Socket* connect();
    int shutdownWrite();

private:
    const InetAddress _serverAddr;
};

}

#endif  
