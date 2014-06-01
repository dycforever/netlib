
#include <string>
#include <map>

class HttpResponse {

private:
    std::map<std::string, std::string> mHeaders;
    std::string mState;
    std::string mDesc;
    std::string mVersion;

    std::string mStr;

    typedef std::map<std::string, std::string>::iterator MapIter;
public:
    HttpResponse() {
       mStr.reserve(512);
    }

    const std::string& toString();
    void setVersion(const std::string&);
    void setState(const std::string&);
    void setDesc(const std::string&);
    void setHeader(const std::string&, const std::string&);

};
