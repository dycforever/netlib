
#ifndef NETLIB_LOG_H
#define NETLIB_LOG_H

#include <errno.h> 
#include <iostream> 
#include <stdio.h> 

enum COLOR{BLACK=0,RED=1,GREEN=2,YELLOW=3,BLUE=4,WHITE=9};
enum Format{BOLD=1,NORMAL,UNDERSCORE=4,REVERSE=7,DELETE=9};
inline void PRINT_COLOR(COLOR foreground,COLOR background=BLACK,Format format=BOLD){
    std::cout<<"\E[3"<<foreground<<";4"<<background<<";"<<format<<"m";
}
inline void UNPRINT_COLOR(){
    std::cout<<"\033[0m";
}

#ifdef LOG_DEBUG

#define LOGOUT stdout
#define NOTICE(format, arguments...) \
    do{ \
        PRINT_COLOR(GREEN); \
        fprintf(LOGOUT," [NOTICE] "); \
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

#define WARNING(format, arguments...) \
    do{ \
        PRINT_COLOR(YELLOW); \
        fprintf(LOGOUT," [WARNING] "); \
        UNPRINT_COLOR(); \
        fprintf(LOGOUT,format"\n", ##arguments);\
        fflush(LOGOUT);\
    }while(0)



#define FATAL(format, arguments...) \
    do{ \
        PRINT_COLOR(RED); \
        fprintf(LOGOUT," [FATAL]  "); \
        UNPRINT_COLOR(); \
        fprintf(LOGOUT,"[%s:%d][%s()] " format"\n", __FILE__, __LINE__, __FUNCTION__, ##arguments); \
        fflush(LOGOUT);\
    } while(0)

#else

#define NOTICE(format, arguments...) void();

#define DEBUG(format, arguments...) void();

#define WARNING(format, arguments...) void();

#define FATAL(format, arguments...) void();

#endif

#endif  // NETLIB_LOG_H
