
#include <alog/Configurator.h>
#include <alog/Logger.h>

#define INFO(format, args...) ALOG_LOG(gLogger, alog::LOG_LEVEL_INFO, format, ##args)
#define ERROR(format, args...) ALOG_LOG(gLogger, alog::LOG_LEVEL_ERROR, format, ##args)
#define DEBUG(format, args...) ALOG_LOG(gLogger, alog::LOG_LEVEL_DEBUG, format, ##args)
#define NOTICE(format, args...) ALOG_LOG(gLogger, alog::LOG_LEVEL_NOTICE, format, ##args)


alog::Logger* gLogger = alog::Logger::getLogger("logtest");

int main() {
    alog::Configurator::configureLogger("./logger.conf");
    INFO("info log");
    ERROR("error log");
    DEBUG(" log");
}
