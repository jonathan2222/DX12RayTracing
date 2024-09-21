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

class rs_hook_sink;
namespace RS
{
	class Logger
	{
	public:
		using Listener = std::function<void(const std::string&, spdlog::level::level_enum)>;

		static void Init();

		static std::shared_ptr<spdlog::logger> GetMultiLogger()
		{
			return s_MultiLogger;
		}

		static void Flush();

		static void AddListener(Listener listener)
		{
			std::lock_guard<std::mutex> lock(s_LoggerMutex);
			s_Callbacks.push_back(listener);
		}

	private:
		static std::shared_ptr<spdlog::logger> s_MultiLogger;
		static std::vector<Listener> s_Callbacks;
		inline static std::mutex s_LoggerMutex;
		friend class rs_hook_sink;
	};
}

#ifdef RS_CONFIG_DEVELOPMENT
#define LOG_DETAILED(file, line, func, level, ...) RS::Logger::GetMultiLogger()->log(spdlog::source_loc{file, line, func}, level, __VA_ARGS__)
	#if LOG_DEBUG_ENABLED
		#define LOG_DEBUG(...)	LOG_DETAILED(__FILE__, __LINE__, SPDLOG_FUNCTION, spdlog::level::debug, __VA_ARGS__)
	#else
		#define LOG_DEBUG(...)
	#endif
#define LOG_INFO(...)		LOG_DETAILED(__FILE__, __LINE__, SPDLOG_FUNCTION, spdlog::level::info, __VA_ARGS__)
#define LOG_WARNING(...)	LOG_DETAILED(__FILE__, __LINE__, SPDLOG_FUNCTION, spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...)		LOG_DETAILED(__FILE__, __LINE__, SPDLOG_FUNCTION, spdlog::level::err, __VA_ARGS__)
#define LOG_CRITICAL(...)	LOG_DETAILED(__FILE__, __LINE__, SPDLOG_FUNCTION, spdlog::level::critical, __VA_ARGS__)
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