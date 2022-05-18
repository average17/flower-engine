#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace flower
{
    constexpr uint32_t FLOWER_ASSET_VERSION = 1;

    extern const std::vector<std::string> GSupportRawTextureFormats;
    extern const std::vector<std::string> GSupportRawMeshFormats;
    extern const std::vector<std::string> GSupportAssetFormats;

    extern bool guessItIsBaseColor(const std::string& path);
    extern bool getAssetPath(const std::string& rawPath, std::string& outMetaFilePath, std::string& outBinFilePath);
    extern bool isProcessedPath(const std::string& path);
    extern bool isSupportFormat(const std::string& path);
    extern bool isSupportTextureFormat(const std::string& path);
    extern bool isSupportMeshFormat(const std::string& path);
    extern bool isHDRTexture(const std::string& path);
    extern bool isMetaFile(const std::string& path);

    // asset compress mode.
    enum class ECompressMode : uint32_t
    {
        Min = 0,
        None,
        LZ4,

        Max,
    };
    extern ECompressMode toCompressMode(uint32_t type);
    extern ECompressMode getDefaultCompressMode();

    enum class EAssetType : uint32_t
    {
        Min = 0,
        Texture,
        Mesh,
        Material,

        Max,
    };
    extern EAssetType toAssetFormat(uint32_t type);

    // asset separate to two part.
    // part 0 is .meta data.
    // part 1 is .bin data.
    struct AssetMeta
    {
        // asset type.
        uint32_t type;

        // asset version.
        uint32_t version;

        // meta json infos.
        std::string json;

        // additional bin data.
        std::vector<char> binBlob;
    };
    extern bool saveMetaFile(const char* path, const AssetMeta& inMetaFile);
    extern bool loadMetaFile(const char* path, AssetMeta& outMetaFile);

    // major bin file on data. detail descriptor on meta file.
    struct AssetBin
    {
        std::vector<char> binBlob;
    };
    extern bool saveBinFile(const char* path, const AssetBin& inBinFile);
    extern bool loadBinFile(const char* path, AssetBin& outBinFile);

    
}