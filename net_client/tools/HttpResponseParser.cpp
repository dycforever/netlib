
#include <fstream>

#include "HttpResponseParser.h"

namespace dyc {

ParseRet HttpResponseParser::parseRespLine(std::string line) {
    size_t vEnd = line.find(' ');
    if (vEnd != std::string::npos) {
        mResp.setVersion(line.substr(0, vEnd));
    } else {
        return WAIT;
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
        mResp.setState(line.substr(codeStart, 3));
    } else {
        return WAIT;
    }

    size_t decsStart = codeStart + 4;
    if (vEnd != std::string::npos) {
        mResp.setDesc(line.substr(decsStart, line.size()-decsStart));
    } else {
        return WAIT;
    }

    return DONE;

}



ParseRet HttpResponseParser::parseRespHeader(std::string line) {
    size_t comma = line.find(":");
    if (comma != std::string::npos) {
        mResp.setHeader(trim(line.substr(0, comma)), 
                trim(line.substr(comma+1, line.size()-1-comma)));
        return DONE;
    }
    return WAIT;
}



int HttpResponseParser::parse(std::string resp, bool isFinish) {
    std::string token;
    size_t start = 0;
    ParseRet ret;
    mResponse.append(resp);
    DEBUG("at first: %lu", mResponse.size());
    if (mPhase == LINE) {
        // TODO if too long, return error
        start = getToken(mResponse, start, token, "\r\n");
        if (start == std::string::npos) {
            DEBUG("read response line return WATI");
            return WAIT;
        }
        ret = parseRespLine(token);
        if (ret == DONE) {
            mPhase = HEADER;
            mResponse = mResponse.substr(start, mResponse.size()-start);
            start = 0;
        } else {
            //                INFO("parse response line return WATI");
            return WAIT;
        }
    }

    DEBUG("after parse header: %lu", mResponse.size());
    while(mPhase == HEADER) {
        size_t ostart = start;
        start = getToken(mResponse, start, token, "\r\n");
        if (start == std::string::npos) {
            DEBUG("read header return WATI");
            return WAIT;
        }
        DEBUG("header line: %s", token.c_str());
        if (token == "") {
            mResponse = mResponse.substr(start, mResponse.size()-start);
            mPhase = BODY;
            start = 0;
            break;
        }
        ret = parseRespHeader(token);
        if (ret != DONE) {
            mResponse = mResponse.substr(ostart, mResponse.size()-ostart);
            INFO("return WATI2");
            return WAIT;
        }
    }
    if (start != 0) {
        mResponse = mResponse.substr(start, mResponse.size()-start);
    }
    ret = mResp.setBody(mResponse);
    if (ret == DONE || isFinish) {
        mPhase = LINE;
        ++mHasParsed;
        return DONE;
    }
    return WAIT;
}

int HttpResponseParser::readData(Buffer& buffer, Buffer& outptuBuffer) {
    size_t size = buffer.readableSize();
    mHasRead += size;
    char* buf = buffer.get(size);
    DEBUG("read %lu bytes data", size);
    std::string resp(buf, size);
    if (parse(resp, buffer.isFinish()) == DONE) {
        mCond.notify();
    }
    return 0;
}

int HttpResponseParser::conn() {
    std::cout << "conn build" << std::endl;
}

std::string HttpResponseParser::out() {
    return mResp.toString();
}

void HttpResponseParser::dump(const std::string& filename) {
    std::ofstream out(filename.c_str());
    out << "chunk: " << mResp.isChunked() << std::endl
        << "content-encoding: " << mResp.getContentEncoding() << std::endl
        << "status code: " << mResp.getStatusCode() << std::endl
        << mResp.bodyToString() << std::endl;
    out.close();
}

void HttpResponseParser::dump() {
    std::cout << "chunk: " << mResp.isChunked() << std::endl
        << "content-encoding: " << mResp.getContentEncoding() << std::endl
        << "status code: " << mResp.getStatusCode() << std::endl
        << mResp.bodyToString() << std::endl;
}

void HttpResponseParser::wait() {
    LockGuard<MutexLock> guard(mLock);
    mCond.wait();
}

}
