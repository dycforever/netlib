
#include "Socket.h"
#include "InetAddress.h"
#include "http_client/HttpRequest.h"

using namespace dyc;

std::string version = "HTTP/1.1";
std::string ae = "gzip";
std::string url = "/s?q=qq&gz2=false";
std::string ip = "42.120.169.24";
std::string port = "80";
std::string host = "m.sp.sm.cn";

int main(int argc, char** argv) {
    InetAddress addr(ip, static_cast<uint16_t>(atoi(port.c_str())));    
    Socket socket(true);
    std::cout << " connect: " << socket.connect(addr) << std::endl;

    InetAddress peerAddr;
    socket.getPeerAddr(peerAddr);
    std::cout << "peer addr: " << peerAddr.toIpPort() << std::endl;

    HttpRequest req;
    req.setUrl(url);
    req.setVersion(version);
    req.setHeader("host", host);
    req.setHeader("Accept-Encoding", ae);
    req.setHeader("Connection", "close");
    req.setHeader("User-Agent", "Mozilla/5.0 (Linux; Android 4.1.1; Nexus 7 Build/JRO03D) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.166  Safari/535.19");

    std::string reqStr = req.toString();

    int ret = -1;
    socket.send(reqStr.c_str(), reqStr.size());

    char buf[1024];
    size_t count = 0;
    ret = socket.recv(buf, 1024);
    while (ret > 0) {
        std::cout << "recv : " << std::string(buf, ret) << std::endl;
        ret = socket.recv(buf, 1024);
        if (count ++ == 5) {
            getchar();
        }
    }
    
    if (ret == 0) {
        std::cout << "connection close" << std::endl;
        return 0;
    } else if (ret < 0 && errno == EAGAIN) {
        std::cout << "close(): " << socket.close();
    } else {
        std::cout << "connection close with:" << strerror(errno) << std::endl;
    }

//    ret = socket.recv(buf, 4);
//    if (ret != 4) {
//        std::cout << "recv " << ret << "bytes" << std::endl;
//    }
//    buf[4] = 0;
//    std::cout << "recv content: " << buf << std::endl;

}
