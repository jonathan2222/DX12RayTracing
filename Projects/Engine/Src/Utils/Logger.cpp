#include "Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/basic_file_sink.h>

#if defined(RS_PLATFORM_WINDOWS) && defined(LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING)
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/pattern_formatter.h>
#endif

#include "Core/CorePlatform.h"

using namespace RS;

std::shared_ptr<spdlog::logger> Logger::s_MultiLogger;
std::vector<Logger::Listener> Logger::s_Callbacks;

#if defined(RS_PLATFORM_WINDOWS) && defined(LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING)
class FilePathFormatter : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg& msg, const std::tm& tm_time, spdlog::memory_buf_t& dest) override
    {
        std::string someTxt = msg.source.filename;
        dest.append(someTxt.data(), someTxt.data() + someTxt.size());
    }

    std::unique_ptr<spdlog::custom_flag_formatter> clone() const override
    {
        return spdlog::details::make_unique<FilePathFormatter>();
    }
};
#endif


class rs_hook_sink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    rs_hook_sink() = default;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
        formatted.push_back('\0'); // add a null terminator for OutputDebugString
#    if defined(SPDLOG_WCHAR_TO_UTF8_SUPPORT)
        //wmemory_buf_t wformatted;
        //details::os::utf8_to_wstrbuf(string_view_t(formatted.data(), formatted.size()), wformatted);
        //OutputDebugStringW(wformatted.data());
#    else
        std::lock_guard<std::mutex> lock(Logger::s_LoggerMutex);
        for (Logger::Listener listener : Logger::s_Callbacks)
            listener(formatted.data(), msg.level);
#    endif
    }

    void flush_() override {}
};

void Logger::Init()
{
    spdlog::set_level(spdlog::level::trace);

#if defined(RS_PLATFORM_WINDOWS) && defined(LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING)
    // Needed to make a new flag pattern for only displaying the filename with its path.
    // For some reason %s does not do that but %@ does. However I do not want the line number after a colon but instead within a parenthesis.
    // This is because I want the double-click functionality to work in VS output window.
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<FilePathFormatter>('*').set_pattern("%^%*(%#) : [%T.%e] [%l] %v%$");
    auto msvcSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    msvcSink->set_level(spdlog::level::trace);
    msvcSink->set_formatter(std::move(formatter));
#endif

    auto rsHookSink = std::make_shared<rs_hook_sink>();
    rsHookSink->set_level(spdlog::level::trace);
    rsHookSink->set_pattern("%^[%T.%e] [%s:%#] %v%$");

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::trace);
    consoleSink->set_pattern("%^[%T.%e] [%s:%#] [%l] %v%$");

    const std::string computerName = CorePlatform::Get()->GetComputerNameStr();
    const std::string configuration = CorePlatform::Get()->GetConfigurationAsStr();
    const std::string logFilePath = Engine::GetDebugFilePath() + "Logs/" + computerName + "_" + configuration + ".txt";

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true);
    fileSink->set_level(spdlog::level::trace);
    fileSink->set_pattern("%^[%T.%e] [%s:%#] [%l] %v%$");

    s_MultiLogger = std::shared_ptr<spdlog::logger>(new spdlog::logger("MultiLogger",
        {
            consoleSink,
            fileSink,
#if defined(RS_PLATFORM_WINDOWS) && defined(LOG_ENABLE_WINDOWS_DEBUGGER_LOGGING)
            msvcSink,
#endif
            rsHookSink
        }));
    s_MultiLogger->set_level(spdlog::level::trace);
    spdlog::register_logger(s_MultiLogger);

    spdlog::flush_every(std::chrono::seconds(LOG_FLUSH_INTERVAL));
}

void RS::Logger::Flush()
{
    s_MultiLogger->flush();
}
