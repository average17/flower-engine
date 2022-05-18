#include "AssetSystem.h"
#include "../Core/UtilsPath.h"
#include "../Async/AsyncThread.h"
#include "../Core/Core.h"
#include "../UI/UISystem.h"
#include "../UI/Icon.h"
#include "AssetTexture.h"
#include "../Async/Taskflow.h"

namespace flower
{
    DelegatesSingleThread<AssetSystem> AssetSystem::onProjectDirDirty {};

    void FolderWatcher::init(const std::string& path)
    {
        m_projectPath = path;
        m_paths.clear();
    }

    bool FolderWatcher::updateFileStatus()
    {
        bool bNeedScan = false;
        std::vector<FileAction> actions = {};

        // loop last time cache paths.
        auto it = m_paths.begin();
        while (it != m_paths.end())
        {
            // last time file exist, but current no exist.
            // maybe delete or move or rename.
            if (!std::filesystem::exists(it->first))
            {
                // name:     it->first
                // status:   FileStatus::Erased
                // editTime: it->second

                // do somthing when file erase here.
                bNeedScan = true;
                {
                    FileAction newAction;
                    {
                        newAction.bInitScan = m_bFirstScan;
                        newAction.bFolder = std::filesystem::is_directory(it->first);
                        newAction.filePath = it->first;
                        newAction.fileStatus = EFileStatus::Erased;
                    }
                    actions.push_back(newAction);
                }

                // also erase old thing.
                it = m_paths.erase(it);
            }
            else
            {
                it++;
            }
        }

        for (auto& file : std::filesystem::recursive_directory_iterator(m_projectPath))
        {
            auto current_file_last_write_time = std::filesystem::last_write_time(file);

            std::filesystem::path fullPath = std::filesystem::absolute(file.path());

            // last time no exist, current exist, file create or move here.
            if (!m_paths.contains(fullPath.string()))
            {
                m_paths[fullPath.string()] = current_file_last_write_time;

                // name:     fullPath.string()
                // status:   FileStatus::Created
                // editTime: current_file_last_write_time

                // do something here
                {
                    FileAction newAction;
                    {
                        newAction.bInitScan  = m_bFirstScan;
                        newAction.bFolder    = std::filesystem::is_directory(fullPath.string());
                        newAction.filePath   = fullPath.string();
                        newAction.fileStatus = EFileStatus::Created;
                    }
                    actions.push_back(newAction);
                }

                bNeedScan = true;
            }
            else
            {
                // file edit time change, file edit.
                if (m_paths[fullPath.string()] != current_file_last_write_time)
                {
                    m_paths[fullPath.string()] = current_file_last_write_time;

                    // name:     fullPath.string()
                    // status:   FileStatus::Modified
                    // editTime: current_file_last_write_time

                    // do something here
                    {
                        FileAction newAction;
                        {
                            newAction.bInitScan  = m_bFirstScan;
                            newAction.bFolder    = std::filesystem::is_directory(fullPath.string());
                            newAction.filePath   = fullPath.string();
                            newAction.fileStatus = EFileStatus::Modified;
                        }
                        actions.push_back(newAction);
                    }

                    bNeedScan = true;
                }
            }
        }

        for(const auto& action : actions)
        {
            m_fileActionsQueue.push(action);
        }

        m_bFirstScan = false;
        return bNeedScan;
    }


	AssetSystem::AssetSystem(ModuleManager* in)
	: IRuntimeModule(in, "AssetSystem")
	{

	}

	bool AssetSystem::init()
	{
        m_textureManager.init(this);
        m_meshManager.init(this);

		// when asset system init.
		// we scan the whole project path.
		m_projectDir = UtilsPath::getUtilsPath()->getProjectPath();

        // set project dir.
        m_folderWatcher.init(m_projectDir);

        // init scan.
        // update file status once on init at main thread.
        m_folderWatcher.updateFileStatus();

        // after folder watcher init.
        // we process file change job as first time scan.
        initFileProcess();

        initFileTableProcess();

        bool result = GSlowAsyncThread.callbacks.registerFunction(m_folderScanASyncWorkName,[this]()
        {
            if(m_folderWatcher.updateFileStatus())
            {
                m_bProjectFolderDirty.store(true);
            }
        });

        if(!result)
        {
            LOG_FATAL("Register asset widget async scan thread fail!");
        }

		return true;
	}

	void AssetSystem::tick(const TickData& tickData)
	{
        if(m_bProjectFolderDirty.load())
        {
            m_bProjectFolderDirty.store(false);

            // broadcast all dirty callbacks.
            onProjectDirDirty.broadcast();
        }

        processFileChangeJob();

        m_textureManager.tick();
        m_meshManager.tick();
	}

	void AssetSystem::release()
	{
        CHECK(GSlowAsyncThread.callbacks.unregisterFunction(m_folderScanASyncWorkName) &&
            "Unregister asset widget async scan thread fail!");

        m_textureManager.release();
        m_meshManager.release();
	}

    void AssetSystem::addAssetMetaFile(std::string path)
    {
        CHECK(std::filesystem::exists(path));
        std::unique_ptr<AssetMeta> newMeta = std::make_unique<AssetMeta>();

        CHECK(!m_allProcessedMetaFiles.contains(path) && "there is repeated add meta file.");
        if(loadMetaFile(path.c_str(),*newMeta))
        {
            EAssetType type = toAssetFormat(newMeta->type);
            switch (type)
            {
            case flower::EAssetType::Texture:
            {
                m_textureManager.addTextureMetaInfo(newMeta.get());
            }
            break;
            case flower::EAssetType::Mesh:
            {
                m_meshManager.addMeshMetaInfo(newMeta.get());
            }
            break;
            case flower::EAssetType::Material:
            {
                m_materialManager.addMaterialFile(newMeta.get());
            }
            break;
            default:
            {
                LOG_WARN("Unkonw asset format in asset meta file {}.", path);
            }
            break;
            }

            m_allProcessedMetaFiles[path] = (EAssetType)newMeta->type;
        }
        else
        {
            LOG_IO_ERROR("Meta file: {} load fail.", path);
        }
    }

    void AssetSystem::erasedAssetMetaFile(std::string path)
    {
        if(m_allProcessedMetaFiles.contains(path))
        {
            auto type = m_allProcessedMetaFiles[path];
            switch (type)
            {
            case flower::EAssetType::Texture:
            {
                m_textureManager.erasedAssetMetaFile(path);
            }
            break;
            case flower::EAssetType::Mesh:
            {
                m_meshManager.erasedAssetMetaFile(path);
            }
            break;
            case flower::EAssetType::Material:
            {
                m_materialManager.erasedMaterialFile(path);
            }
            break;
            default:
            {
                LOG_WARN("Unkonw asset format in asset meta file {}.", path);
            }
            break;
            }

            m_allProcessedMetaFiles.erase(path);
        }
    }

    void AssetSystem::processRawFile(std::string path)
    {
        // when raw file exist, .meta and .bin file should no exist.
        // we delete they if they still exist.
        std::string metaFile{};
        std::string binFile{};
        if(getAssetPath(path, metaFile, binFile))
        {
            // preprocess old cache data.
            if(std::filesystem::exists(metaFile))
            {
                LOG_IO_WARN("Raw path: {0} and meta file: {1} exist same time, delete old meta file.", path, metaFile);
                // this action will toggle file erased.
                // handle on tick.
                std::filesystem::remove(metaFile);
            }
            if(std::filesystem::exists(binFile))
            {
                LOG_IO_WARN("Raw path: {0} and bin file: {1} exist same time, delete old bin file.", path, binFile);
                // this action will toggle file erased.
                // handle on tick.
                std::filesystem::remove(binFile);
            }

            // texture process.
            if(isSupportTextureFormat(path))
            {
                if(isHDRTexture(path))
                {
                    // todo: hdr load.
                    
                }
                else
                {
                    // ldr texture load.
                    std::string metaFile{};
                    std::string binFile{};
                    if(getAssetPath(path,metaFile,binFile))
                    {
                        bakeTexture2DLDR(
                            path.c_str(),
                            metaFile.c_str(),
                            binFile.c_str(),
                            getDefaultCompressMode(),
                            getDefaultTextureComponentsCount(),
                            getDefaultTextureFormat(),
                            getDefaultSamplerType(),
                            true,
                            guessItIsBaseColor(path) ? 0.75f : 1.0f
                        );

                        // when bake texture ready we remove raw file.
                        std::filesystem::remove(path);
                    }
                }
            }

            // mesh process.
            if(isSupportMeshFormat(path))
            {
                std::string metaFile{};
                std::string binFile{};
                if(getAssetPath(path,metaFile,binFile))
                {
                    bakeAssimpMesh(
                        path.c_str(),
                        metaFile.c_str(),
                        binFile.c_str()
                    );

                    // when bake texture ready we remove raw file.
                    std::filesystem::remove(path);
                }
            }
        }
    }

    void AssetSystem::processRawFileAsync(std::string path)
    {
        GTaskExecutor.silent_async([this, path]{
            processRawFile(path);
        });
    }

    void AssetSystem::initFileProcess()
    {
        // dump all init actions produced by init scan.
        std::vector<FileAction> ativeAction{};
        while(true)
        {
            auto action = m_folderWatcher.m_fileActionsQueue.pop();
            if(!action)
            {
                break;
            }
            ativeAction.push_back(*action);
        }

        for(const auto& action : ativeAction)
        {
            CHECK(action.bInitScan && action.fileStatus == EFileStatus::Created && 
                "Init action should only call on initFileProcess");

            CHECK((!m_allSupportFiles.contains(action.filePath)) && "Files hash has repeat key!");
            
            if(isSupportFormat(action.filePath))
            {
                // insert all support format path on cache.
                m_allSupportFiles.emplace(action.filePath);
            }
        }
    }

    void AssetSystem::initFileTableProcess()
    {
        GTaskExecutor.wait_for_all();
        for(const auto& path : m_allSupportFiles)
        {
            // raw file process.
            if(!isProcessedPath(path))
            {
                processRawFileAsync(path);            
            }
        }
        GTaskExecutor.wait_for_all();

        for(const auto& path : m_allSupportFiles)
        {
            // load meta files.
            if(isMetaFile(path))
            {
                if(!m_allProcessedMetaFiles.contains(path))
                {
                    addAssetMetaFile(path);
                }
            }
        }
    }

    void AssetSystem::processFileChangeJob()
    {
        std::vector<FileAction> ativeAction{};
        while(true)
        {
            auto action = m_folderWatcher.m_fileActionsQueue.pop();
            if(!action)
            {
                break;
            }
            ativeAction.push_back(*action);
        }

        // sync all file change.
        for(const auto& action : ativeAction)
        {
            CHECK(!action.bInitScan && "you should not call this function on init scan.");

            switch(action.fileStatus)
            {
            case EFileStatus::Created:
                processFileCreated(action);
                break;
            case EFileStatus::Erased:
                processFileErased(action);
                break;
            case EFileStatus::Modified:
                processFileModified(action);
                break;
            default:
                LOG_FATAL("Unprocess file action here. fix me.");
                break;
            }
        }
    }

    void AssetSystem::processFileCreated(const FileAction& action)
    {
        if(action.bFolder)
        {
            // folder create.
        }
        else
        {
            // file create.
            if(isSupportFormat(action.filePath))
            {
                CHECK((!m_allSupportFiles.contains(action.filePath)) && "Files hash has repeat key!");
                m_allSupportFiles.emplace(action.filePath);

                if(isProcessedPath(action.filePath))
                {
                    // processed file create need load meta file.
                    if(isMetaFile(action.filePath))
                    {
                        addAssetMetaFile(action.filePath);
                    }
                }
                else
                {
                    // raw file create need process.
                    processRawFileAsync(action.filePath);
                }
            }
        }
    }

    void AssetSystem::processFileErased(const FileAction& action)
    {
        if(action.bFolder)
        {
            // folder erased.
        }
        else
        {
            if(isSupportFormat(action.filePath))
            {
                // erase files on m_allSupportFiles.
                m_allSupportFiles.erase(action.filePath);

                // try to erase meta file if exist.
                erasedAssetMetaFile(action.filePath);
            }
        }
    }

    void AssetSystem::processFileModified(const FileAction& action)
    {
        if(action.bFolder)
        {
            // folder modify.
        }
        else
        {
            // file modify. need reload.
            if(isSupportFormat(action.filePath))
            {
                // it's not a good idea to change bin file or meta file from disk.
                // you should only edit on engine.
                // it's easy to handle these case.
                
                // asset file edit, need reload.
                if(isProcessedPath(action.filePath))
                {
                    if(isMetaFile(action.filePath))
                    {
                        // meta file modify, need reload.
                        // todo:
                    }
                    else
                    {
                        // bin file modify, also need reload.
                        // todo:
                    }
                }
            }
        }
    }

    UISystem* AssetSystem::getUISystem()
    {
        if(m_uiSystem) return m_uiSystem;

        m_uiSystem = m_moduleManager->getRuntimeModule<UISystem>();
        return m_uiSystem;
    }
}

