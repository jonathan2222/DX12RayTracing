#include "Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/ansicolor_sink.h>

using namespace RS;

std::shared_ptr<spdlog::logger> Logger::s_ConsoleLogger;

void Logger::Init()
{
    spdlog::set_level(spdlog::level::info);

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);
    consoleSink->set_pattern("%^[%T.%e] [%s:%#] [%l] %v%$");
    s_ConsoleLogger = std::make_shared<spdlog::logger>("ConsoleLogger", consoleSink);
    spdlog::register_logger(s_ConsoleLogger);
}