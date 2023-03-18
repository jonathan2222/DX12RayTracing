#pragma once

#pragma warning( push )
#pragma warning( disable : 26451 )
#pragma warning( disable : 26439 )
#pragma warning( disable : 26495 )
#pragma warning( disable : 4244 )
#if defined(RS_CONFIG_DEVELOPMENT) && defined(LOG_DEBUG_ENABLED)
	#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif
#include <spdlog/spdlog.h>
#pragma warning( pop )

#include <memory>

namespace RS
{
	class Logger
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger> GetMultiLogger()
		{
			return s_MultiLogger;
		}

		static void Flush();

	private:
		static std::shared_ptr<spdlog::logger> s_MultiLogger;
	};
}

#ifdef RS_CONFIG_DEVELOPMENT
	#if LOG_DEBUG_ENABLED
		#define LOG_DEBUG(...)	SPDLOG_LOGGER_DEBUG(RS::Logger::GetMultiLogger(), __VA_ARGS__)
	#else
		#define LOG_DEBUG(...)
	#endif
#define LOG_INFO(...)		SPDLOG_LOGGER_INFO(RS::Logger::GetMultiLogger(), __VA_ARGS__)
#define LOG_WARNING(...)	SPDLOG_LOGGER_WARN(RS::Logger::GetMultiLogger(), __VA_ARGS__)
#define LOG_ERROR(...)		SPDLOG_LOGGER_ERROR(RS::Logger::GetMultiLogger(), __VA_ARGS__)
#define LOG_CRITICAL(...)	SPDLOG_LOGGER_CRITICAL(RS::Logger::GetMultiLogger(), __VA_ARGS__)
#define LOG_SUCCESS(...)	LOG_INFO(__VA_ARGS__)
#define LOG_FLUSH(...)		RS::Logger::Flush()
#else
#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_WARNING(...)
#define LOG_ERROR(...)
#define LOG_CRITICAL(...)
#define LOG_SUCCESS(...)
#define LOG_SUCCESS(...)
#endif