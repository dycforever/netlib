#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils/StringTokenizer.h"
#include "utils/StringUtil.h"

namespace dyc
{

long getRecvQueueLength(int fd) 
{
    struct stat buf;
    int ret = fstat(fd, &buf);
    if (ret != 0) {
        return -1;
    }
    ino_t inode = buf.st_ino;
    std::ifstream fin("/proc/net/tcp");
    std::string readLine;
    while(getline(fin, readLine)) {
        StringTokenizer tokenizer;
        tokenizer.Tokenize(readLine, " ", StringTokenizer::TOKEN_IGNORE_EMPTY | StringTokenizer::TOKEN_TRIM);
        if (ToString(inode) == tokenizer[9]) {
            std::string srq = tokenizer[4];
            std::stringstream ss;
            ss << srq.substr(srq.find(":") + 1);
            long ret;
            ss >> ret;
            return ret;
        }
    }
}

}
