
#ifndef NETLIB_LOG_H
#define NETLIB_LOG_H

#include <errno.h> 
#include <iostream> 
#include <stdio.h> 


#ifdef SHENMA

#include <alog/Configurator.h>
#include <alog/Logger.h>

#define INFO(format, args...) ALOG_LOG(gLogger, alog::LOG_LEVEL_INFO, format, ##args)
#define WARN(format, args...) ALOG_LOG(gLogger, alog::LOG_LEVEL_WARN, format, ##args)
#define FATAL(format, args...) ALOG_LOG(gLogger, alog::LOG_LEVEL_FATAL, format, ##args)
#define DEBUG(format, args...) ALOG_LOG(gLogger, alog::LOG_LEVEL_DEBUG, format, ##args)

extern alog::Logger* gLogger;

#else // not at SHENMA


enum COLOR{BLACK=0,RED=1,GREEN=2,YELLOW=3,BLUE=4,WHITE=9};
enum Format{BOLD=1,NORMAL,UNDERSCORE=4,REVERSE=7,DELETE=9};
inline void PRINT_COLOR(COLOR foreground,COLOR background=BLACK,Format format=BOLD){
    std::cout<<"\E[3"<<foreground<<";4"<<background<<";"<<format<<"m";
}
inline void UNPRINT_COLOR(){
    std::cout<<"\033[0m";
}

#define LOGOUT stdout
#ifdef LOG_DEBUG

#define INFO(format, arguments...) \
    do{ \
        PRINT_COLOR(GREEN); \
        fprintf(LOGOUT," [INFO] "); \
        UNPRINT_COLOR(); \
        fprintf(LOGOUT,format"\n", ##arguments);\
        fflush(LOGOUT);\
    }while(0)

#define DEBUG(format, arguments...) \
    do{ \
        PRINT_COLOR(BLUE); \
        fprintf(LOGOUT," [DEBUG] "); \
        UNPRINT_COLOR(); \
        fprintf(LOGOUT,format"\n", ##arguments);\
        fflush(LOGOUT);\
    }while(0)

#define WARN(format, arguments...) \
    do{ \
        PRINT_COLOR(YELLOW); \
        fprintf(LOGOUT," [WARN] "); \
        UNPRINT_COLOR(); \
        fprintf(LOGOUT,format"\n", ##arguments);\
        fflush(LOGOUT);\
    }while(0)


#else

#define INFO(format, arguments...) void();

#define DEBUG(format, arguments...) void();

#define WARN(format, arguments...) void();

#endif


#define FATAL(format, arguments...) \
    do{ \
        PRINT_COLOR(RED); \
        fprintf(LOGOUT," [FATAL]  "); \
        UNPRINT_COLOR(); \
        fprintf(LOGOUT,"[%s:%d][%s()] " format"\n", __FILE__, __LINE__, __FUNCTION__, ##arguments); \
        fflush(LOGOUT);\
    } while(0)


#endif // SHENMA


#endif  // NETLIB_LOG_H
