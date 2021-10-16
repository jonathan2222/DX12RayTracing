#pragma once

#pragma warning( push )
#pragma warning( disable : 26451 )
#pragma warning( disable : 26439 )
#pragma warning( disable : 26495 )
#pragma warning( disable : 4244 )
#include <spdlog/spdlog.h>
#pragma warning( pop )

#include <memory>

namespace RS
{
	class Logger
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger> GetConsoleLogger()
		{
			return s_ConsoleLogger;
		}
	private:
		static std::shared_ptr<spdlog::logger> s_ConsoleLogger;
	};
}

#ifdef RS_CONFIG_DEVELOPMENT
#define LOG_INFO(...)		SPDLOG_LOGGER_INFO(RS::Logger::GetConsoleLogger(), __VA_ARGS__)
#define LOG_WARNING(...)	SPDLOG_LOGGER_WARN(RS::Logger::GetConsoleLogger(), __VA_ARGS__)
#define LOG_ERROR(...)		SPDLOG_LOGGER_ERROR(RS::Logger::GetConsoleLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...)	SPDLOG_LOGGER_CRITICAL(RS::Logger::GetConsoleLogger(), __VA_ARGS__)
#define LOG_SUCCESS(...)	LOG_INFO(__VA_ARGS__)
#else
#define LOG_INFO(...)
#define LOG_WARNING(...)
#define LOG_ERROR(...)
#define LOG_CRITICAL(...)
#define LOG_SUCCESS(...)
#endif