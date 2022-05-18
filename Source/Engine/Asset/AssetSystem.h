#pragma once
#include "../Core/RuntimeModule.h"
#include "../Async/LockFreeSPSCQueue.h"
#include "../Core/Delegates.h"

#include "AssetCommon.h"
#include "AssetTexture.h"
#include "AssetMesh.h"
#include "AssetMaterial.h"

#include <unordered_map>
#include <filesystem>
#include <map>
#include <set>
#include <functional>

namespace flower
{
	enum class EFileStatus
	{
		Created,  // file create or move here.
		Modified, // file edit.
		Erased,   // file delete, move, or rename.
	};

	struct FileAction
	{
		bool bInitScan;
		bool bFolder;
		EFileStatus fileStatus;
		std::string filePath;
	};

	class AssetSystem;
	class UISystem;

	class FolderWatcher
	{
		friend AssetSystem;
	private:
		bool m_bFirstScan = true;

		std::string m_projectPath;

		// cache project paths, read and write only on watcher thread.
		std::unordered_map<std::string, std::filesystem::file_time_type> m_paths;

		// lock-free action queue, push onlu on watcher thread, pop only on assetsystem thread.
		LockFreeSPSCQueue<FileAction> m_fileActionsQueue;

	public:
		FolderWatcher() = default;

		// call on main thread.
		void init(const std::string& path);

		// call on async thread, return value is that file is dirty.
		bool updateFileStatus();
	};

	class AssetSystem : public IRuntimeModule
	{
	public:
		AssetSystem(ModuleManager* in);

		virtual bool init() override;
		virtual void tick(const TickData&) override;
		virtual void release() override;

	private:
		std::atomic_bool m_bProjectFolderDirty = true;
		std::string m_projectDir;

		const char* m_folderScanASyncWorkName = "ProjectFolderScanAsyncWork";
		FolderWatcher m_folderWatcher;

		// all support format file path store set.
		std::set<std::string> m_allSupportFiles;

		// all meta files already load, use for load unload check.
		std::map<std::string, EAssetType> m_allProcessedMetaFiles;

		// asset managers.
		TextureManager m_textureManager{};
		MeshManager m_meshManager{};
		MaterialManager m_materialManager{};

		// ui system reference.
		UISystem* m_uiSystem = nullptr;

	private:
		// add new meta file. also flash m_assetTypeMap.
		void addAssetMetaFile(std::string path);

		// remove meta file. also flash m_assetTypeMap.
		void erasedAssetMetaFile(std::string path);

		// disptach job to process raw file.
		void processRawFile(std::string path);
		void processRawFileAsync(std::string path);

		// init process.
		void initFileProcess();

		// init file table. process missing bin/meta situation.
		void initFileTableProcess();

		// consume all file change actions and do process on tick.
		void processFileChangeJob();

		// file actions.
		void processFileCreated(const FileAction& action);
		void processFileErased(const FileAction& action);
		void processFileModified(const FileAction& action);

	public:
		static DelegatesSingleThread<AssetSystem> onProjectDirDirty;
		TextureManager* getTextureManager() { return &m_textureManager; }
		MeshManager* getMeshManager() { return &m_meshManager; }
		MaterialManager* getMaterialManager() { return &m_materialManager; }
		UISystem* getUISystem();
	};
}