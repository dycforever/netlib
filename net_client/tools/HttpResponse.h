
#ifndef __HTTPRESPONSE_H__
#define __HTTPRESPONSE_H__

#include "common.h"

namespace dyc {

enum ParseRet {DONE, WAIT};

class HttpResponse {

private:
    std::map<std::string, std::string> mHeaders;
    std::string mStatusCode;
    std::string mDesc;
    std::string mVersion;

    std::string mBody;

    bool mChunked;
    bool mGzip;
    std::string mStr;
    size_t mContentLength;

    //TODO use pointer instead
    std::vector<std::string> mChunks;
    int strtosize(const std::string& str);
    int char2num(char c);

    ParseRet parseChunk(std::string& b);
    typedef std::map<std::string, std::string>::iterator MapIter;
public:
    HttpResponse():mBody(""), mChunked(false), mGzip(false), mContentLength(0),
                   mStatusCode(""), mDesc(""), mVersion("") {
       mStr.reserve(512);
    }

    std::string judgeHeader(char* header, int size);

    size_t isChunked();
    std::string getStatusCode() {return mStatusCode;}
    std::string getContentType() {return mHeaders["content-type"];}
    std::string getContentEncoding() {return mHeaders["content-encoding"];}
    std::string bodyToString();

    const std::string& toString();
    void dealChunk(const std::string& chunk, const std::string& ,std::string&);
    void setVersion(const std::string&);
    void setState(const std::string&);
    void setDesc(const std::string&);
    void setHeader(const std::string&, const std::string&);
    ParseRet setBody(std::string&);

};

}

#endif