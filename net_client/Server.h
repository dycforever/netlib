#ifndef SERVER_H
#define SERVER_H

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
#include "Accepter.h"

namespace dyc {

class Server {
public:
    typedef boost::function< int (Buffer*, Buffer*) > ReadCallbackFunc;
    typedef boost::function< int (Buffer*) > WriteCallbackFunc;

    typedef Connection* ConnectionPtr;
//    typedef std::map<InetAddress, Connection*> ConnectionCollections;
    typedef std::set<ConnectionPtr> ConnectionCollections;

    Server(const InetAddress& listenAddr);
    ~Server();  

    int start();
    void stop();

    Connection* connect(const InetAddress&);
    void removeConnection(const Socket& conn);
    Connection* newConnection(Socket*);

    const InetAddress& getAddress() const { return mListenAddr; }

    void setReadCallback(ReadCallbackFunc cb) { mReadCallback = cb;}
    void setWriteCallback(WriteCallbackFunc cb) { mWriteCallback = cb;}

private:
    const InetAddress mListenAddr;
    Epoller*  mEpoller;
    Socket*  mListenSocket;
    EventLoop* mLoop; 
    Accepter* mAccepter;
//    boost::shared_ptr<Epoller> mEpoller;
//    boost::scoped_ptr<Socket> mListenSocket;
//    boost::shared_ptr<EventLoop> mLoop;

    WriteCallbackFunc mWriteCallback;
    ReadCallbackFunc mReadCallback;

    ConnectionCollections mConnections;
};

}

#endif  
