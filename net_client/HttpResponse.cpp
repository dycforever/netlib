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
    do {
        strm.avail_out = size;
        strm.next_out = out;
//        printHeader((unsigned char*)strm.next_in, strm.avail_in, "strm.avail_in");
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
    } while (ret != Z_STREAM_END && strm.avail_in != 0);
    (void)inflateEnd(&strm);
    delete[] out;
    return res;
}

std::string HttpResponse::judgeHeader(char* header, int size) {
    if (header[0] == 0x00 && header[1] == 0x01 && header[2] == 0x02) {
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

void HttpResponse::dealChunk(const std::string& chunk, const std::string& ret, std::string& str) {
    char* header = const_cast<char*>(chunk.c_str());
    assert(header != NULL);
    if (ret == "gzip" || ret == "gz2") {
        std::string gztxt = decode(header, chunk.size());
        str.append("\n" + ret + " content: \n").append(gztxt);
    } else {
        str.append("\n" + ret + " content: \n").append(chunk);
    }
}


std::string HttpResponse::bodyToString() {
    std::string retStr;
    if (mChunked && mChunks.size() > 0) {
        char* header = const_cast<char*>(mChunks[0].c_str());
        std::string ret = judgeHeader(header, mChunks[0].size());
        std::string totalChunk;
        for (int i=0; i<mChunks.size(); ++i) {
            totalChunk += mChunks[i];
            // FIXME
//            dealChunk(mChunks[i], ret, retStr);
        }
        std::string txt;
        if (mGzip) {
            txt = decode(totalChunk.c_str(), totalChunk.size());
            retStr.append("\n" + ret + " content: \n").append(txt);
        } else {
            retStr.append("\n" + ret + " content: \n").append(totalChunk);
        }
    } else { // not chunk
        char* header = const_cast<char*>(mBody.c_str());
        std::string ret = judgeHeader(header, mBody.size());
        dealChunk(mBody, ret, retStr);
    }
    return retStr;
}

const std::string& HttpResponse::toString() {
    mStr.clear();
    mStr.append(mVersion).append(" ").append(mState).append(" ").append(mDesc).append("\n");
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
    std::string key = strToLower(k);
    mHeaders[key] = v;
    std::string lv = strToLower(v);
    std::transform(lv.begin(), lv.end(), lv.begin(), ::tolower);
    if (key == "transfer-encoding" && lv == "chunked") {
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
    ParseRet ret = WAIT;
    while(1) {
        size_t ostart = start;
        start = getToken(b, start, token, "\r\n");
        if (start == std::string::npos) {
            std::cout << "not enough chunk size" << std::endl;
            b = b.substr(ostart, b.size()-ostart);
            break;
        }
        size_t chunkExt = token.find(";");
        if (chunkExt == std::string::npos) {
            chunkExt = token.size();
        }
        std::string chunkSizeStr(token.c_str(), chunkExt);
        int size = strtosize(chunkSizeStr);
        std::cout << "get a chunk Size: " << size << std::endl;
        if (size == 0) {
            ret = DONE;
            break;
        } else if (size < 0) {
//            std::cout << "token: " << token << " size : " << token.size()<< std::endl;
//            std::cout << "start: " << start << " char: " << token[start]<< std::endl;
            b = b.substr(ostart, b.size()-ostart);
            ret = WAIT;
            break;
        }
        size_t chunkSize = size;
        if (b.begin()+start+chunkSize >= b.end()) {
            std::cout << "not enough chunk" << std::endl;
            b = b.substr(ostart, b.size()-ostart);
            break;
        }
        std::string chunk(b.begin()+start, b.begin()+start+chunkSize);
        start += chunkSize + 2; // there is CRLF following a chunk
        mChunks.push_back(chunk);
    }
    return ret;
}

ParseRet HttpResponse::setBody(std::string& b) {
    std::cout << "body size:" << b.size() << std::endl;
    ParseRet ret = DONE;
    mBody = b;
    if (mChunked) {
        ret = parseChunk(b);
        mBody = b;
    } 
    return ret;
}

