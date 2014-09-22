
#ifndef __HTTPRESPONSEPARSER_H__
#define __HTTPRESPONSEPARSER_H__

#include "common.h"
#include "thread/Condition.h"

#include "Buffer.h"
#include "http_client/HttpResponse.h"
#include "netutils/Tokenizer.h"
#include "netutils/Log.h"

namespace dyc {

class HttpResponseParser {
private:
    enum ParsePhase {LINE, HEADER, BODY};
public:
    HttpResponseParser():mResponseBuf(""), mHasRead(0), mHasParsed(0), mCond(mLock), mPhase(LINE)
{
    mTest = false;
}

    ParseRet parseRespLine(const std::string& line);
    ParseRet parseRespHeader(const std::string& line);
    int parse(const std::string& resp, bool isFinish);
    int readData(Buffer* buffer, Buffer*);
    int conn(bool);
    std::string out();
    HttpResponse& getResponse();

    void wait();

private:
    std::string mResponseBuf;
    int mHasParsed;
    size_t mHasRead;
    MutexLock mLock;
    Condition mCond;
    HttpResponse mResponseObj;
    ParsePhase mPhase;

    bool mTest;
    
};

}

#endif

