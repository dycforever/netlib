
#include "Socket.h"
#include "InetAddress.h"

using namespace std;
using namespace dyc;

int main() {
    InetAddress addr("192.168.1.106", 8714);
    Socket listener(true);
    int ret = listener.bind(addr);
    cout << ret << endl;

    ret = listener.listen();
    cout << ret << endl;
    char buf[1024];

    InetAddress peerAddr;
    int fd = listener.accept(peerAddr);
    cout << "peerAddr: "  << peerAddr.toIpPort() << endl;
    Socket socket(fd);
    
    getchar();
    ret = socket.recv(buf, 1);
    size_t read = 0;
    while ( ret > 0) {
        read += ret;
        cout << "read " << read<< " bytes" << endl;
        getchar();
        ret = socket.recv(buf, 1);
    }
}
