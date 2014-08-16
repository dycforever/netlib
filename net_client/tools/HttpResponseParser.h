
#ifndef __HTTPRESPONSEPARSER_H__
#define __HTTPRESPONSEPARSER_H__

#include "common.h"
#include "thread/Condition.h"

#include "Buffer.h"
#include "tools/HttpResponse.h"
#include "netutils/Tokenizer.h"
#include "netutils/Log.h"

namespace dyc {

class HttpResponseParser {
private:
    enum ParsePhase {LINE, HEADER, BODY};
public:
    HttpResponseParser():mResponse(""), mHasRead(0), mHasParsed(0), mCond(mLock), mPhase(LINE)
{
    mTest = false;
}

    ParseRet parseRespLine(std::string line);
    ParseRet parseRespHeader(std::string line);
    int parse(std::string resp, bool isFinish);
    int readData(Buffer* buffer, Buffer*);
    int conn();
    std::string out();
    void dump(const std::string& filename) ;
    void dump();
    void wait();

private:
    std::string mResponse;
    int mHasParsed;
    size_t mHasRead;
    MutexLock mLock;
    Condition mCond;
    HttpResponse mResp;
    ParsePhase mPhase;

    bool mTest;
    
};

}

#endif

