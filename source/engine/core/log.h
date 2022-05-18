#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <deque>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "NonCopyable.h"

namespace flower 
{
	enum class ELogType : size_t
	{
		Trace = 0,
		Info,
		Warn,
		Error,
		Other,

		Max,
	};

	// custom log cache sink, use for editor/hub/custom console output. etc.
	template<typename Mutex>
	class LogCacheSink : public spdlog::sinks::base_sink <Mutex>
	{
	private:
		std::vector<std::function<void(std::string, ELogType)>> m_callbacks;

		static ELogType toLogType(spdlog::level::level_enum level)
		{
			switch (level)
			{
			case spdlog::level::trace:
				return ELogType::Trace;
			case spdlog::level::info:
				return ELogType::Info;
			case spdlog::level::warn:
				return ELogType::Warn;
			case spdlog::level::err:
			case spdlog::level::critical:
				return ELogType::Error;
			default:
				return ELogType::Other;
			}
		}

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override
		{
			spdlog::memory_buf_t formatted;
			spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
			for (auto& fn : m_callbacks)
			{
				if (fn)
				{
					fn(fmt::to_string(formatted), toLogType(msg.level));
				}
			}
		}

		void flush_() override
		{

		}

	public:
		int pushCallback(std::function<void(std::string, ELogType)> callback)
		{
			m_callbacks.push_back(callback);
			return int(m_callbacks.size()) - 1;
		}

		void popCallback(int id)
		{
			m_callbacks[id] = nullptr;
		}
	};

	class Logger : private NonCopyable
	{
	private:
		static  std::shared_ptr<Logger> s_instance;

		std::shared_ptr<spdlog::logger> m_loggerUtil;
		std::shared_ptr<spdlog::logger> m_loggerIO;
		std::shared_ptr<spdlog::logger> m_loggerGraphics;
		std::shared_ptr<LogCacheSink<std::mutex>> m_loggerCache;

		void init();

	public:
		Logger();

	public:
		 inline static std::shared_ptr<Logger>&       getInstance(){ return s_instance; }
		inline std::shared_ptr<spdlog::logger>&     getLoggerUtil(){ return m_loggerUtil; }
		inline std::shared_ptr<spdlog::logger>&       getLoggerIo(){ return m_loggerIO; }
		inline std::shared_ptr<spdlog::logger>& getLoggerGraphics(){ return m_loggerGraphics; }
		inline std::shared_ptr<LogCacheSink<std::mutex>>& getLoggerCache() { return m_loggerCache; }
	};
}
