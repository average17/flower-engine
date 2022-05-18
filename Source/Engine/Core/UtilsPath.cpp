#include <filesystem>
#include "../Engine.h"
#include "UtilsPath.h"
#include "Core.h"

namespace flower 
{
	void UtilsPath::setProjectPath(const std::string& projectPath)
	{
		CHECK((!GEngine.isEngineInit()) && "you can only set project path before engine init.");

		m_projectDir = projectPath;
	}

	UtilsPath* UtilsPath::getUtilsPath()
	{
		static UtilsPath path{ };
		return &path;
	}

	const std::string UtilsPath::getEngineWorkingPath() const
	{
		return std::filesystem::current_path().string() + "/";
	}

	const std::string UtilsPath::getEngineLogFileFolderPath() const
	{
		return getEngineWorkingPath() + m_logfileFolderPath;
	}
}