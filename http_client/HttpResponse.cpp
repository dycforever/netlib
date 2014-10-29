#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "http_client/HttpResponse.h"

#include "netutils/Tokenizer.h"
#include "netutils/netutils.h"

#include "zlib.h"

namespace dyc {

HttpResponse::HttpResponse() :
    mBodyBuf(""), 
    mChunked(false), 
    mGzip(false), 
    mContentLength(0),
    mRestContentLength(0),
    mStatusCode(""), 
    mDesc(""), 
    mVersion("") {
    mStr.reserve(512);
}

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

std::string HttpResponse::judgeHeader(char* header, int size) {
    if (header[0] == 0x00 && header[1] == 0x01 && header[2] == 0x08) {
        header[0] = (char)0x1f;
        header[1] = (char)0x8b;
        header[2] = (char)0x08;
        mGzip = true;
        return "gz2";
    } else if (header[0] == (char)0x1f && header[1] == (char)0x8b && header[2] == (char)0x08) {
        mGzip = true;
        return "gzip";
    } else {
        return "text";
    }
}

size_t HttpResponse::isChunked() {
    if (!mChunked) {
        return 0;
    } else {
        return mChunks.size();
    }
}

void HttpResponse::decodeChunk(const std::string& chunk, const std::string& contentType, std::string& str) {
    char* header = const_cast<char*>(chunk.c_str());
    assert(header != NULL);
    if (contentType == "gzip" || contentType == "gz2") {
        std::string gztxt = decode(header, chunk.size());
        str.append("\n" + contentType + " content: \n").append(gztxt);
    } else {
        str.append("\n" + contentType + " content: \n").append(chunk);
    }
}

std::string HttpResponse::bodyToString() {
    std::string retStr;
    if (mChunked && mChunks.size() > 0) {
        char* header = const_cast<char*>(mChunks[0].c_str());
        std::string contentType = judgeHeader(header, mChunks[0].size());
        std::string totalChunk;
        for (int i=0; i<mChunks.size(); ++i) {
            totalChunk += mChunks[i];
            // FIXME
            std::stringstream ss;
            ss << "chunk_"<< i;
            std::ofstream fout(ss.str().c_str());
            fout << mChunks[i];
//            decodeChunk(mChunks[i], contentType, retStr);
        }
        std::string txt;
        if (mGzip) {
            txt = decode(totalChunk.c_str(), totalChunk.size());
            retStr.append("\n" + contentType + " content : \n").append(txt);
        } else {
            retStr.append("\n" + contentType + " content : \n").append(totalChunk);
        }
    } else { // not chunk
        char* header = const_cast<char*>(mBodyBuf.c_str());
        std::string contentType = judgeHeader(header, mBodyBuf.size());
        decodeChunk(mBodyBuf, contentType, retStr);
    }
    return retStr;
}

const std::string& HttpResponse::toString() {
    mStr.clear();
    mStr.append(mVersion).append(" ").append(mStatusCode).append(" ").append(mDesc).append("\n");
    for (MapIter iter = mHeaders.begin(); iter != mHeaders.end(); ++iter) {
        mStr.append(iter->first).append(" ==> ").append(iter->second).append("\n");
    }
    mStr += bodyToString();
    return mStr;
}

std::string strToLower(const std::string& str) {
    std::string ret = str;
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

void HttpResponse::setHeader(const std::string& k, const std::string& v) {
    std::cerr << "set header key: " << k << " value: " << v << std::endl;
    std::string key = strToLower(k);
    mHeaders[key] = v;
    if (key == "transfer-encoding" && v == "chunked") {
        mChunked = true;
    }
    if (key == "content-length") {
        mContentLength = atoi(v.c_str());
        mRestContentLength = mContentLength;
    }
}

void HttpResponse::setVersion(const std::string& v) {
    mVersion = v;
}

void HttpResponse::setState(const std::string& s) {
    mStatusCode = s;
}

void HttpResponse::setDesc(const std::string& d) {
    mDesc = d;
}

int HttpResponse::char2num(char c) {
    if (c <= 'f' && c >= 'a') {
        return c-'a' + 10;
    } else if (c <= 'F' && c >= 'A') {
        return c-'A' + 10;
    } else if (c <= '9' && c >= '0') {
        return c-'0';
    } else {
        return -1;
    }
}

int HttpResponse::strtosize(const std::string& str) {
//    std::cout << "parsing hex str:" << str << std::endl;
    const char* buf = str.c_str();
    size_t size = str.size();
    int ret = -1;
    for (int i=0; i < size; i++) {
        if (ret == -1) {
            ret = 0;
        }
        int num = char2num(buf[i]);
        if (num >= 0) {
            ret = ret * 16 + num;
        } else {
            return -1;
        }
    }
    return ret;
}

ParseRet HttpResponse::parseChunk(std::string& b) {
    size_t start = 0;
    std::string token;
    ParseRet ret = PARSE_WAIT;
    while(1) {
        size_t ostart = start;
        start = getToken(b, start, token, "\r\n");
        if (start == std::string::npos) {
//            WARN("not enough chunk size");
            b = b.substr(ostart, b.size()-ostart);
            ret = PARSE_WAIT;
            std::cerr << "xx 1" << std::endl;
            break;
        }
        size_t chunkExt = token.find(";");
        if (chunkExt == std::string::npos) {
            std::cerr << "chunkExt: " << chunkExt << std::endl;
            chunkExt = token.size();
        }
        std::string chunkSizeStr(token.begin(), token.begin() + chunkExt);
        int size = strtosize(chunkSizeStr);
        DEBUG_LOG("get a chunk Size: %s", chunkSizeStr.c_str());
        std::cerr << "get a chunk Size: " << chunkSizeStr.c_str() 
            << " => " << size << std::endl;
        if (size == 0) {
            ret = PARSE_DONE;
            std::cerr << "xx 2" << std::endl;
            break;
        } else if (size < 0) {
//            b = b.substr(ostart, b.size()-ostart);
            ret = PARSE_ERROR;
            std::cerr << "xx 3" << std::endl;
            break;
        }
        size_t chunkSize = size;
        if (b.begin()+start+chunkSize >= b.end()) {
            b = b.substr(ostart, b.size()-ostart);
            ret = PARSE_WAIT;
            std::cerr << "xx 4" << std::endl;
            break;
        }
        std::string chunk(b.begin()+start, b.begin()+start+chunkSize);
        start += chunkSize + 2;
        mChunks.push_back(chunk);
    }
    return ret;
}

ParseRet HttpResponse::appendBody(const std::string& block) {
    DEBUG_LOG("append body size: %lu, mChunked: %d", block.size(), (int)mChunked);
    ParseRet ret = PARSE_DONE;
    mBodyBuf.append(block);
    if (mChunked) {
        ret = parseChunk(mBodyBuf);
    } else {
        if (mRestContentLength <= block.size()) {
            mRestContentLength = 0;
            return PARSE_DONE;
        }
        mRestContentLength -= block.size();
        return PARSE_WAIT;
    }
    return ret;
}

void HttpResponse::dump(const std::string& filename) {
    std::ofstream out(filename.c_str());
    out << "chunk: " << isChunked() << std::endl
        << "content-encoding: " << getContentEncoding() << std::endl
        << "status code: " << getStatusCode() << std::endl
        << bodyToString() << std::endl;
    out.close();
}

void HttpResponse::dump() {
    std::cout << "chunk: " << isChunked() << std::endl
        << "content-encoding: " << getContentEncoding() << std::endl
        << "status code: " << getStatusCode() << std::endl
        << bodyToString() << std::endl;
}

}
