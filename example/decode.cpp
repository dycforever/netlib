
#include "common.h" 
#include "netutils/Log.h"
#include "zlib.h" 

namespace dyc 
{
std::string decode(const char* data, size_t size)
{
    std::string res;
    int ret;
    size_t have;
    z_stream strm;
    unsigned char* out = NEW unsigned char[size];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, 32+15);
    if (ret != Z_OK) {
        FATAL_LOG("inflateInit2 failed");
        return "";
    }

    strm.avail_in = size;
    strm.next_in = (unsigned char*)data;
    do {
        strm.avail_out = size;
        strm.next_out = out;
//        printHeader((unsigned char*)strm.next_in, strm.avail_in, "strm.avail_in");
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        switch (ret) {
            case Z_NEED_DICT:
                FATAL_LOG("inflate return Z_NEED_DICT");
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
                FATAL_LOG("inflate return Z_DATA_ERROR" );
            case Z_MEM_ERROR:
                FATAL_LOG("inflate return Z_MEM_ERROR");
                (void)inflateEnd(&strm);
                FATAL_LOG("inflate failed");
                return "";
        }
        have = size- strm.avail_out;
        res.append((const char*)out, have);
    } while (ret != Z_STREAM_END && strm.avail_in != 0);
    (void)inflateEnd(&strm);
    INFO_LOG("decode %lu bytes", size);
    delete[] out;
    return res;
}
}

int main()
{
    return 0;
}
