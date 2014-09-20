
#ifndef __HTTPRESPONSE_H__
#define __HTTPRESPONSE_H__

#include "common.h"

namespace dyc {

enum ParseRet {PARSE_DONE, PARSE_WAIT, PARSE_ERROR};

class HttpResponse {
public:
    HttpResponse();

    std::string judgeHeader(char* header, int size);
    size_t isChunked();
    std::string getStatusCode() {return mStatusCode;}
    std::string getContentType() {return mHeaders["content-type"];}
    std::string getContentEncoding() {return mHeaders["content-encoding"];}

    void decodeChunk(const std::string& chunk, const std::string& ,std::string&);
    void setVersion(const std::string&);
    void setState(const std::string&);
    void setDesc(const std::string&);
    void setHeader(const std::string&, const std::string&);
    ParseRet appendBody(const std::string&);

public:
    std::string bodyToString();
    const std::string& toString();
    void dump(const std::string& filename) ;
    void dump();

private:
    std::map<std::string, std::string> mHeaders;
    std::string mStatusCode;
    std::string mDesc;
    std::string mVersion;

    std::string mBodyBuf;

    bool mChunked;
    bool mGzip;
    std::string mStr;
    size_t mContentLength;
    size_t mRestContentLength;

    //TODO use pointer instead
    std::vector<std::string> mChunks;
    int strtosize(const std::string& str);
    int char2num(char c);

    ParseRet parseChunk(std::string& b);
    typedef std::map<std::string, std::string>::iterator MapIter;

};

}

#endif
