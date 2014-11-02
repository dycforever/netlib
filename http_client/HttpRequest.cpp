
#include "HttpRequest.h"

namespace dyc {

const std::string& HttpRequest::toString() {
    mStr.append(mMethod).append(" ").append(mUrl).append(" ").append(mVersion).append("\r\n");
    for (MapIter iter = mHeaders.begin(); iter != mHeaders.end(); ++iter) {
        mStr.append(iter->first).append(": ").append(iter->second).append("\r\n");
    }
    mStr.append("\r\n");
}

void HttpRequest::setHeader(const std::string& k, const std::string& v) {
    mHeaders[k] = v;
}

void HttpRequest::setUrl(const std::string& url) {
    mUrl = url;
}

void HttpRequest::setMethod(const std::string& m) {
    mMethod = m;
}

void HttpRequest::setVersion(const std::string& v) {
    mVersion = v;
}

}
