#include "Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

#if defined(RS_PLATFORM_WINDOWS) && defined(LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING)
#include <spdlog/sinks/msvc_sink.h>
#endif

#include "Core/CorePlatform.h"

using namespace RS;

std::shared_ptr<spdlog::logger> Logger::s_MultiLogger;

void Logger::Init()
{
    spdlog::set_level(spdlog::level::trace);

#if defined(RS_PLATFORM_WINDOWS) && defined(LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING)
    auto msvcSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    msvcSink->set_level(spdlog::level::trace);
    msvcSink->set_pattern("%^[%T.%e] [%s:%#] [%l] %v%$");
#endif

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);
    consoleSink->set_pattern("%^[%T.%e] [%s:%#] [%l] %v%$");

    const std::string computerName = CorePlatform::Get()->GetComputerNameStr();
    const std::string configuration = CorePlatform::Get()->GetConfigurationAsStr();
    const std::string logFilePath = RS_LOG_FILE_PATH + computerName + "_" + configuration + ".txt";

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true);
    fileSink->set_level(spdlog::level::trace);
    fileSink->set_pattern("%^[%T.%e] [%s:%#] [%l] %v%$");

    s_MultiLogger = std::shared_ptr<spdlog::logger>(new spdlog::logger("MultiLogger",
        {
            consoleSink,
            fileSink
#if defined(RS_PLATFORM_WINDOWS) && defined(LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING)
            ,msvcSink
#endif
        }));
    s_MultiLogger->set_level(spdlog::level::trace);
    spdlog::register_logger(s_MultiLogger);

    spdlog::flush_every(std::chrono::seconds(LOG_FLUSH_INTERVAL));
}

void RS::Logger::Flush()
{
    s_MultiLogger->flush();
}
