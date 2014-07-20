
#include "Socket.h"
#include "InetAddress.h"

using namespace std;
using namespace dyc;

int main() {
    InetAddress addr("192.168.1.106", 8714);
    Socket socket(true);
    int ret = socket.connect(addr);
    InetAddress peerAddr;
    socket.getPeerAddr(peerAddr);
    cout << ret << ": " << peerAddr.toIpPort() << endl;
    getchar();
    char* buf = new char[29714240];
    ret = socket.send(buf, 29714240);
    size_t writen = 0;
    while ( ret > 0) {
        writen += ret;
        cout << "send " << writen << " bytes" << endl;
        ret = socket.send(buf, 29714240);
    }
}