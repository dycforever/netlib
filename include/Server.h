#ifndef Server_H
#define Server_H

#include <map>
#include <vector>
#include <list>
#include <set>
#include <algorithm>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "InetAddress.h" 
#include "Socket.h" 
#include "Epoller.h"
#include "EventLoop.h"
#include "Connection.h"
#include "Task.h"

namespace dyc {

class Server {
public:
    typedef boost::shared_ptr<SendTask> SendTaskPtr;
    typedef boost::function< SendTaskPtr (RecvTask*) > ReadCallbackFunc;
    typedef boost::function< int (Socket*) > ConnCallbackFunc;
    typedef boost::function<char* (const Head&)> MallocCallbackFunc;
    typedef boost::function<int (SendTaskPtr)> WriteCallbackFunc;

    typedef Connection* ConnectionPtr;
    Server(const InetAddress& listenAddr);
    ~Server();  

    const InetAddress& host_address() const { return _listenAddr; }

    int start();
    void stop();

    void remove_connection(const Socket& conn);

    int accepter(Socket* sock);
    Connection* newConnection(Socket*);

    void setReadCallback(ReadCallbackFunc cb) { _readCallback = cb;}
    void setWriteCallback(WriteCallbackFunc cb) { _writeCallback = cb;}
    void setMallocCallback(MallocCallbackFunc cb) { _mallocCallback = cb;}

    Connection* connect(const InetAddress&);
private:

//    typedef std::map<InetAddress, Connection*> ConnectionCollections;
    typedef std::set<ConnectionPtr> ConnectionCollections;
    const InetAddress _listenAddr;
    boost::shared_ptr<Epoller> _epoller;
    boost::scoped_ptr<Socket> _listenSocket;
    boost::shared_ptr<EventLoop> _loop;

    MallocCallbackFunc _mallocCallback;
    WriteCallbackFunc _writeCallback;
    ReadCallbackFunc _readCallback;
    ReadCallbackFunc _errorCallback;

    ConnectionCollections _connections;
};

}

#endif  
