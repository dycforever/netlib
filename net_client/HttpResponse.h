
#include <string>
#include <map>
#include <vector>

class HttpResponse {

private:
    std::map<std::string, std::string> mHeaders;
    std::string mState;
    std::string mDesc;
    std::string mVersion;

    std::string mBody;

    bool mChunked;
    std::string mStr;

    //TODO 
    std::vector<std::string> mChunks;

    void parseChunk(const std::string& b);
    typedef std::map<std::string, std::string>::iterator MapIter;
public:
    HttpResponse():mBody(""), mChunked(false),
                   mState(""), mDesc(""), mVersion("") {
       mStr.reserve(512);
    }

    const std::string& toString();
    void setVersion(const std::string&);
    void setState(const std::string&);
    void setDesc(const std::string&);
    void setHeader(const std::string&, const std::string&);
    void setBody(const std::string&);

};
