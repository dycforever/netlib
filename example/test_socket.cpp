
//
// include test_check()/test_linger()/test_shutdown()
//

#include "common.h"
#include "Socket.h"
#include "InetAddress.h"

using namespace dyc;

void checkSocket(Socket* socket)
{
    bool ret = socket->checkConnected();
    std::cout << "socket: " << (ret ? "true" : "false") << std::endl;
    socket->setNonblocking();
    while (1) {
        char c;
        long count = socket->recv(&c, 1, NULL);
        if (count != -1) {
            std::cout << "socket recv " << count << " bytes" << std::endl;
        }

        bool tmpRet = socket->checkConnected();
        if (ret != tmpRet) {
            std::cout << "socket change to " << (tmpRet? "true" : "false") << std::endl;
        }
        ret = tmpRet;
        if (count == 0) {
            break;
        }
    }
    std::cout << "before close, socket " << (socket->checkConnected()? "true" : "false") << std::endl;
    socket->close();
    std::cout << "after close, socket " << (socket->checkConnected()? "true" : "false") << std::endl;
}

void test_check() {
    Socket* mListenSocket = NEW Socket(true);
    assert(mListenSocket != NULL);

    std::string ip = "0.0.0.0";
    std::string port = "8714";
    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
    int ret = mListenSocket->bind(addr);
    assert(ret != -1);
    ret = mListenSocket->listen();
    assert(ret != -1);

    std::cout << "will accept" << std::endl;
    int newfd = mListenSocket->accept(addr);

    Socket newso(newfd);
    newso.setLinger(true, 0);

    checkSocket(&newso);
}

void test_linger() {
    Socket* mListenSocket = NEW Socket(true);
    assert(mListenSocket != NULL);

    std::string ip = "0.0.0.0";
    std::string port = "8714";
    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
    int ret = mListenSocket->bind(addr);
    assert(ret != -1);
    ret = mListenSocket->listen();
    assert(ret != -1);

    std::cout << "will accept" << std::endl;
    int newfd = mListenSocket->accept(addr);

    Socket newso(newfd);
    newso.setLinger(true, 0);

    char buf[200];
    memset(buf, 0, 200);
    std::cout << "recv " << newso.recv(buf, 78, NULL) << " bytes" << std::endl;

    std::string resp = "dyc resp";
    std::cout << "will send" << std::endl;
    newso.send(resp);

    std::cout << "will close" << std::endl;
    sleep(2);
    newso.close();
    std::cout << "closed" << std::endl;
}

void test_shutdown()
{
    Socket* mListenSocket = NEW Socket(true);
    assert(mListenSocket != NULL);

    std::string ip = "0.0.0.0";
    std::string port = "8714";
    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
    int ret = mListenSocket->bind(addr);
    assert(ret != -1);
    ret = mListenSocket->listen();
    assert(ret != -1);

    std::cout << "will accept" << std::endl;
    int newfd = mListenSocket->accept(addr);

    Socket newso(newfd);
    newso.setNonblocking();
    while (1) {
        char c;
        long count = newso.recv(&c, 1, NULL);
        if (count == 0) {
            std::cout << "socket recv 0 bytes" << std::endl;
            break;
        }
    }
    newso.close();
}

void test_buf()
{
    Socket socket(true);
    socket.setSendBuf(100);
    int bufsize = 0;
    socklen_t retsize = 0;
    socket.getSendBuf(&bufsize, &retsize);
    std::cout << bufsize << " " << retsize << std::endl;
}

int main()
{
    // if linger.timeout == 0, send rst in all situation
    // test_linger();
    
    // if countpart send FIN. the socket is valid before close(), even read() return 0
    // id countpart send RST, same with FIN(all cause read() return 0)
    // test_check();

    // test_shutdown();
    test_buf();
    while(1) {
        sleep(1);
    }
}
