#include <stdlib.h>

#include "HttpResponse.h"

const std::string& HttpResponse::toString() {
    mStr.append(mVersion).append(" ").append(mState).append(" ").append(mDesc).append("\n");
    mStr.append("headers: \n");
    for (MapIter iter = mHeaders.begin(); iter != mHeaders.end(); ++iter) {
        mStr.append("*").append(iter->first).append(" ==> ").append(iter->second).append("\n");
    }
    mStr.append("\n");
}

void HttpResponse::setHeader(const std::string& k, const std::string& v) {
    mHeaders[k] = v;
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

