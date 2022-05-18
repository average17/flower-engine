#pragma once
#include <string>
#include <mutex>

namespace flower 
{ 
	// global path for application.
	// only set value when application init.
	class UtilsPath
	{
	private:
		const std::string m_logfileFolderPath = "./Log/";
		const std::string m_fontFolderPath    = "./Font/";
		const std::string m_settingPath       = "./Setting/";

		const std::string m_iconFilePath = "./Image/flower.png";

		std::string m_projectDir = "./Project/";

	public:
		// return engine working path, end with /.
		// eg: C:/local/app.exe get path is
		//     C:/local/
		// so we can append other utils path by simply plus.
		const std::string getEngineWorkingPath() const;
		const std::string getEngineLogFileFolderPath() const;
		const std::string getEngineIconFilePath() const { return m_iconFilePath; }
		const std::string getEngineFontFolderPath() const { return m_fontFolderPath; }
		const std::string getEngineSettingPath() const { return m_settingPath; }
	
		const std::string getProjectPath() const { return m_projectDir; }

		// set working project path.
		// you should only call before engine init.
		void setProjectPath(const std::string& projectPath);

	private:
		UtilsPath() = default;

	public:
		static UtilsPath* getUtilsPath();
	};

	namespace fontAsset
	{
		inline static std::string deng = "fa-brands-800.ttf";
	}

}