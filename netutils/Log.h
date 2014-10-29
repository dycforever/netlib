#ifndef __NETLIB_LOG_H__
#define __NETLIB_LOG_H__

#include <iostream>
#include "log4c.h"

namespace dyc {

const char* errno2str(int errno_p);

extern log4c_category_t* gDYCLogObj;

class DYC_GLOBAL {
public:
    DYC_GLOBAL() {
        log4c_init();  
        gDYCLogObj = log4c_category_get("dyclog");
    }

    ~DYC_GLOBAL() {
        log4c_fini();
    }
};

#define DEBUG_LOG(format, arguments...) \
    do{ \
        log4c_category_log(gDYCLogObj, LOG4C_PRIORITY_DEBUG, format, ##arguments);  \
    }while(0)

#define TRACE_LOG(format, arguments...) \
    do{ \
        log4c_category_log(gDYCLogObj, LOG4C_PRIORITY_TRACE, format, ##arguments);  \
    }while(0)

#define INFO_LOG(format, arguments...) \
    do{ \
        log4c_category_log(gDYCLogObj, LOG4C_PRIORITY_INFO, format, ##arguments);  \
    }while(0)

#define NOTICE_LOG(format, arguments...) \
    do{ \
        log4c_category_log(gDYCLogObj, LOG4C_PRIORITY_NOTICE, format, ##arguments);  \
    }while(0)

#define WARN_LOG(format, arguments...) \
    do{ \
        log4c_category_log(gDYCLogObj, LOG4C_PRIORITY_WARN, format, ##arguments);  \
    } while(0)

#define FATAL_LOG(format, arguments...) \
    do{ \
        log4c_category_log(gDYCLogObj, LOG4C_PRIORITY_FATAL, "[%s:%d][%s()] " format, __FILE__, __LINE__, __FUNCTION__, ##arguments);  \
    } while(0)

#define FATAL_ERROR(format, arguments...) \
    do{ \
        log4c_category_log(gDYCLogObj, LOG4C_PRIORITY_FATAL, "[%s:%s][%s:%d][%s()] " format, errno2str(errno) ,strerror(errno) , __FILE__, __LINE__, __FUNCTION__, ##arguments);  \
    } while(0)


} // namespace

#endif // __NETLIB_LOG_H__
