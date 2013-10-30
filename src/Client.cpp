
#include "Client.h"
#include "Socket.h"

namespace dyc {

Socket* Client::connect() {
    int sockfd = createBlockingSocket();
    Socket* socket = NEW Socket(sockfd);

    int ret = socket->connect(_serverAddr);
    assert(ret != -1);
    return socket;
}


}
