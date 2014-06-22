
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "common.h"
#include "Socket.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "EventLoop.h"
#include "Server.h"

using namespace std;
using namespace dyc;

// global options:
std::string ip = "127.0.0.1";
std::string port = "8714";

int parseArg(int argc, char** argv) {
    int c;
    while((c = getopt(argc, argv, "v:u:a:h:p:i:")) != -1) {
        switch(c) {
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            default:
                std::cerr << "parse options failed" << std::endl;
                return -1;
        }
    }
    return 0;
} // parseArg()

int getConnected(Buffer& buffer, Buffer& outputBuffer) {
    size_t size = buffer.readableSize();
    string mesg(buffer.get(size), size);
    cout << "recv " << size << "bytes data: " << mesg << endl;
    outputBuffer.append("hello response");
    return 0;
}

int main() {
    typedef boost::function< int (Buffer&, Buffer&) > ReadCallbackFunc;

    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));
    Server server(addr);
    ReadCallbackFunc func = boost::bind(&getConnected, _1, _2);
    server.setReadCallback(func);
    server.start();

    return 0;
}