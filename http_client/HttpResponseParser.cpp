
#include <fstream>

#include "HttpResponseParser.h"

namespace dyc {

ParseRet HttpResponseParser::parseRespLine(const std::string& line) {
    size_t vEnd = line.find(' ');
    if (vEnd != std::string::npos) {
        mResponseObj.setVersion(line.substr(0, vEnd));
    } else {
        return PARSE_WAIT;
    }

    size_t codeStart = (vEnd + 1);
    if (vEnd != std::string::npos) {
        if (line.substr(codeStart, 3) != "200") {
            std::ofstream out("debug.out");
            out << "line: " << line << std::endl 
                << "###############\n" 
                << "code start: \n" << codeStart << " vEnd : " << vEnd << std::endl;
            out.close();

            assert(line.substr(codeStart, 3) == "200");
        }
        mResponseObj.setState(line.substr(codeStart, 3));
    } else {
        return PARSE_WAIT;
    }

    size_t decsStart = codeStart + 4;
    if (vEnd != std::string::npos) {
        mResponseObj.setDesc(line.substr(decsStart, line.size()-decsStart));
    } else {
        return PARSE_WAIT;
    }
    return PARSE_DONE;
}

ParseRet HttpResponseParser::parseRespHeader(const std::string& line) {
    size_t comma = line.find(":");
    if (comma != std::string::npos) {
        mResponseObj.setHeader(trim(line.substr(0, comma)), 
                trim(line.substr(comma+1, line.size()-1-comma)));
        return PARSE_DONE;
    }
    return PARSE_WAIT;
}


int HttpResponseParser::parse(const std::string& resp, bool isFinish) {
    std::string token;
    size_t start = 0;
    ParseRet ret;
    mResponseBuf.append(resp);
    if (mPhase == LINE) {
        // TODO if too long, return error
        start = getToken(mResponseBuf, start, token, "\r\n");
        if (token.size() >= 10240) {
            WARN_LOG("response line too long: %s", token.c_str());
            return PARSE_ERROR;
        }
        if (start == std::string::npos) {
            DEBUG_LOG("read response line return WATI");
            return PARSE_WAIT;
        }
        ret = parseRespLine(token);
        if (ret == PARSE_DONE) {
            mPhase = HEADER;
        } else {
            INFO_LOG("parse response line return WATI");
            return PARSE_WAIT;
        }
    }

    DEBUG_LOG("after parse header: %lu", mResponseBuf.size());
    while(mPhase == HEADER) {
        size_t originStart = start;
        start = getToken(mResponseBuf, start, token, "\r\n");
        if (start == std::string::npos) {
            mResponseBuf = mResponseBuf.substr(originStart, mResponseBuf.size()-originStart);
            DEBUG_LOG("read header return WATI");
            return PARSE_WAIT;
        }
        DEBUG_LOG("header line: %s", token.c_str());
        if (token == "") {
            mPhase = BODY;
            break;
        }
        ret = parseRespHeader(token);
        if (ret != PARSE_DONE) {
            INFO_LOG("invalid header: %s", token.c_str());
        }
    }
    ret = mResponseObj.appendBody(mResponseBuf.substr(start, mResponseBuf.size()-start));
    mResponseBuf.clear();

    if (ret == PARSE_DONE || isFinish) {
        ++mHasParsed;
        return PARSE_DONE;
    }
    return ret;
}

// dyc: read call back
int HttpResponseParser::readData(Buffer* buffer, Buffer* outptuBuffer) {
    size_t size = buffer->readableSize();
    mHasRead += size;
    char* buf = buffer->get(size);
    DEBUG_LOG("read %lu bytes data, finish[%d]", size, (int)buffer->isFinish());
    std::string resp(buf, size);
    if (parse(resp, buffer->isFinish()) == PARSE_DONE) {
        LockGuard<MutexLock> guard(mLock);
        mDone = true;
        DEBUG_LOG("done to notify");
        mCond.notify();
    }
    return 0;
}

int HttpResponseParser::conn(bool success) {
    std::cout << "conn build " 
        << (success ? " success" : " failed")
        << std::endl;
}

void HttpResponseParser::wait() {
    LockGuard<MutexLock> guard(mLock);
    DEBUG_LOG("will wait");
    while (!mDone) {
        mCond.wait();
    }
    DEBUG_LOG("wait done");
    return;
}

HttpResponse& HttpResponseParser::getResponse() {
    return mResponseObj;
}

}
