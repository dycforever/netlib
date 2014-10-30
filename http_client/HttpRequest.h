
#ifndef __HTTPREQUEST_H__
#define __HTTPREQUEST_H__

#include <map>
#include <string>

namespace dyc {

class HttpRequest {
private:
    std::map<std::string, std::string> mHeaders;
    std::string mMethod;
    std::string mUrl;
    std::string mVersion;

    std::string mStr;

    typedef std::map<std::string, std::string>::iterator MapIter;
public:
    HttpRequest()
        : mMethod("GET"), 
          mUrl("/"), 
          mVersion("HTTP/1.1"){
       mStr.reserve(512);
    }

    const std::string& toString();
    void setVersion(const std::string&);
    void setMethod(const std::string&);
    void setUrl(const std::string&);
    void setHeader(const std::string&, const std::string&);

};

}

#endif

