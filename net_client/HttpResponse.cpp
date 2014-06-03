#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <algorithm>

#include "HttpResponse.h"
#include "Tokenizer.h"
#include "util.h"

#include "zlib.h"

void printHeader(const unsigned char* buf, size_t size, const std::string& pref) {
        std::cout << pref << ": " << (void*) buf 
            << " size: " << size << " => " 
            << (int)buf[0] << " " 
            << (int)buf[1] << " " 
            << (int)buf[2] << " " << std::endl;
}

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
        FATAL("inflateInit2 failed");
        return "";
    }

    strm.avail_in = size;
    strm.next_in = (unsigned char*)data;
    FILE* fp = fopen("zdata","w");

    fwrite(strm.next_in, 1, strm.avail_in, fp);

    do {
        strm.avail_out = size;
        strm.next_out = out;
        printHeader((unsigned char*)strm.next_in, strm.avail_in, "strm.avail_in");
        std::cout << "hehe" << std::endl;
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
        switch (ret) {
            case Z_NEED_DICT:
                std::cout << "inflate return Z_NEED_DICT" << std::endl;
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
                std::cout << "inflate return Z_DATA_ERROR" << std::endl;
            case Z_MEM_ERROR:
                std::cout << "inflate return Z_MEM_ERROR" << std::endl;
                (void)inflateEnd(&strm);
                FATAL("inflate failed");
                return "";
        }
        have = size- strm.avail_out;
        res.append((const char*)out, have);

//        std::cout << " ===================" << std::endl;
//        std::cout << "append " << have << ", res: " << res << std::endl;

    } while (ret != Z_STREAM_END);
    fclose(fp);

    /* done when inflate() says it's done */

    /* clean up and return */
    (void)inflateEnd(&strm);
    delete[] out;


    res.append("\0", 1);
    return res;
}


const std::string& HttpResponse::toString() {
    mStr.append(mVersion).append(" ").append(mState).append(" ").append(mDesc).append("\n");
    for (MapIter iter = mHeaders.begin(); iter != mHeaders.end(); ++iter) {
        mStr.append(iter->first).append(" ==> ").append(iter->second).append("\n");
    }
    if (mBody[0] == 0x00 && mBody[1] == 0x01 && mBody[2] == 0x02) {
        mStr.append("a gz2 body: \n");
        mBody[0] = (char)0x1f;
        mBody[1] = (char)0x8b;
        mBody[2] = (char)0x08;
        std::string gztxt = decode(mBody.c_str(), mBody.size());
        mStr.append(gztxt);
    } else if (mBody[0] == (char)0x1f && mBody[1] == (char)0x8b && mBody[2] == (char)0x08) {
        mStr.append("a gzip body: \n");
        std::string gztxt = decode(mBody.c_str(), mBody.size());
        mStr.append(gztxt);
    } else {
        mStr.append("text body:");
        mStr.append(mBody);
    }
    return mStr;
}

void HttpResponse::setHeader(const std::string& k, const std::string& v) {
    mHeaders[k] = v;
    std::string tmpk = k;
    std::string tmpv = v;
    std::transform(tmpk.begin(), tmpk.end(), tmpk.begin(), ::tolower);
    std::transform(tmpv.begin(), tmpv.end(), tmpv.begin(), ::tolower);
    if (tmpk == "transfer-encoding" && tmpv == "chunked") {
        mChunked= true;
    }

}

void HttpResponse::setVersion(const std::string& v) {
    mVersion = v;
}

void HttpResponse::setState(const std::string& s) {
    mState = s;
}

void HttpResponse::setDesc(const std::string& d) {
    mDesc = d;
}

std::string parseChunk(const std::string& b) {
    size_t start = 0;
    std::string token;
    std::string res;
    while(1) {
        start = getToken(b, start, token, "\r\n");
        if (start == std::string::npos) {
            break;
        }
        size_t chunkExt = token.find(";");
        if (chunkExt == std::string::npos) {
            chunkExt = token.size();
        }
        std::string chunkSizeStr(token.c_str(), chunkExt);
        size_t chunkSize = strtol(chunkSizeStr.c_str(), NULL, 16);
//        std::cout << "token: " << token << " chunkSize: " << chunkSize << std::endl;
        if (chunkSize == 0)
            break;
        std::string chunk(b.begin()+start, b.begin()+start+chunkSize);
        start += chunkSize;
        res += chunk;
    }
    return res;
}

void HttpResponse::setBody(const std::string& b) {
    mBody = b;
    if (mChunked) {
        std::cout << "parsing chunk !!" << std::endl;
        std::string body = parseChunk(b);
        mBody = body;
//        printHeader((unsigned char*)mBody, mBodySize, "mBody");
//        std::cout << "mBody: " << (void*) mBody  << " size: " << mBodySize 
//            << " => " 
//            << (int)mBody[0] << " " 
//            << (int)mBody[1] << " " 
//            << (int)mBody[2] << " " << std::endl;
        std::ofstream out("body.out");
        out << body;
        out.close();
        return;
    } 
    std::ofstream out("out");
    out << b;
    out.close();
}

