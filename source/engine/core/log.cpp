#include <filesystem>
#include <sstream>
#include <deque>
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Log.h"
#include "CVar.h"
#include "UtilsPath.h"

namespace flower
{
	static AutoCVarInt32 cVarLogFileOutEnable(
		"r.Log.FileOutEnable",
		"Enable log file output. 0 is off, 1 is on.",
		"Log",
		1,
		CVarFlags::ReadOnly | CVarFlags::InitOnce
	);

	static AutoCVarString cVarLogFileName(
		"r.Log.OutFileName",
		"Set log file output name, it only work when cVarLogFileOutEnable == 1.",
		"Log",
		"flower",
		CVarFlags::ReadOnly | CVarFlags::InitOnce
	);

	static AutoCVarInt32 cVarLogWithThreadId(
		"r.Log.LogWithThreadId",
		"Enable log with thread id. 0 is off, 1 is on.",
		"Log",
		0,
		CVarFlags::ReadOnly | CVarFlags::InitOnce
	);

	std::shared_ptr<Logger> Logger::s_instance = std::make_shared<Logger>();

	Logger::Logger()
	{
		init();
	}

	const auto s_PrintFormat         = "%^[%H:%M:%S][%l] %n: %v%$";
	const auto s_PrintFormatWithId   = "%^[%H:%M:%S][thread id:%t][%l] %n: %v%$";
	const auto s_LogFileFormatWithId =   "[%H:%M:%S][thread id:%t][%l] %n: %v";
	const auto s_LogFileFormat       =   "[%H:%M:%S][%l] %n: %v";
	const auto s_CachePrintFormat    = "%^[%H:%M:%S][%l] %n: %v%$";

	using TimePoint = std::chrono::system_clock::time_point;
	inline std::string serializeTimePoint(const TimePoint& time, const std::string& format)
	{
		std::time_t tt = std::chrono::system_clock::to_time_t(time);
		std::tm tm = *std::localtime(&tt);
		std::stringstream ss;
		ss << std::put_time(&tm, format.c_str());
		return ss.str();
	}

	void Logger::init()
	{
		if (s_instance != nullptr)
			return;

		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

		m_loggerCache = std::make_shared<LogCacheSink<std::mutex>>();
		logSinks.emplace_back(m_loggerCache);

		cVarLogWithThreadId.get() != 0 ?
			logSinks[0]->set_pattern(s_PrintFormatWithId) :
			logSinks[0]->set_pattern(s_PrintFormat);

		logSinks[1]->set_pattern(s_CachePrintFormat);

		if (cVarLogFileOutEnable.get() != 0)
		{
			std::filesystem::create_directory(UtilsPath::getUtilsPath()->getEngineLogFileFolderPath());
			auto path = UtilsPath::getUtilsPath()->getEngineLogFileFolderPath() + cVarLogFileName.get();

			TimePoint input = std::chrono::system_clock::now();
			path += serializeTimePoint(input, "%Y-%m-%d %H_%M_%S") + ".log";

			logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true));

			cVarLogWithThreadId.get() != 0 ?
				logSinks[logSinks.size() - 1]->set_pattern(s_LogFileFormatWithId) :
				logSinks[logSinks.size() - 1]->set_pattern(s_LogFileFormat);
		}

		m_loggerUtil = std::make_shared<spdlog::logger>("Util", begin(logSinks), end(logSinks));
		spdlog::register_logger(m_loggerUtil);
		m_loggerUtil->set_level(spdlog::level::trace);
		m_loggerUtil->flush_on(spdlog::level::trace);

		m_loggerIO = std::make_shared<spdlog::logger>("IO", begin(logSinks), end(logSinks));
		spdlog::register_logger(m_loggerIO);
		m_loggerIO->set_level(spdlog::level::trace);
		m_loggerIO->flush_on(spdlog::level::trace);

		m_loggerGraphics = std::make_shared<spdlog::logger>("Graphics", begin(logSinks), end(logSinks));
		spdlog::register_logger(m_loggerGraphics);
		m_loggerGraphics->set_level(spdlog::level::trace);
		m_loggerGraphics->flush_on(spdlog::level::trace);
	}
}