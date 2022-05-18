#pragma once
#include <map>
#include "AssetCommon.h"
#include "../Renderer/Mesh.h"
#include "../RHI/Common.h"

namespace flower
{
    struct MeshInfo
    {
        struct MeshBounds
        {
            float origin[3];
            float radius;
            float extents[3];
        };

        struct SubMeshInfo
        {
            // uuid of searialize material.
            std::string materialPath;

            MeshBounds bounds = {};
            uint32_t indexCount;
            uint32_t indexStartPosition;
            uint32_t vertexCount;
        };

        uint32_t subMeshCount = 0;
        std::vector<SubMeshInfo> subMeshInfos = {};
        uint32_t vertCount;
        std::vector<EVertexAttribute> attributeLayout = {};

        ECompressMode compressMode;

        // Load file path.
        std::string originalFile;

        // UUID of this asset.
        std::string UUID;
    };

    // current mesh no exist meta additional info, future we will add screenshot.
    extern int32_t getSize(const std::vector<EVertexAttribute>& layouts);
    extern int32_t getCount(const std::vector<EVertexAttribute>& layouts);
    extern bool readMeshInfo(MeshInfo& info, AssetMeta* file);
    extern bool writeMesh(MeshInfo* info, AssetMeta* metaFile, AssetBin* binFile, char* vertexData, char* indexData, bool bCompress);
    extern void unpackMeshBinFile(MeshInfo* info, const char* sourcebuffer, size_t sourceSize, std::vector<float>& vertexBufer, std::vector<VertexIndexType>& indexBuffer);
    extern bool bakeAssimpMesh(const char* pathIn, const char* metaPathOut, const char* binPathOut, bool bCompress = true);
    
    namespace engineMeshes
    {
        struct EngineMeshInfo
        {
            std::string path;
            std::string displayName;
            std::string snapShotPath;

            bool operator==(const EngineMeshInfo& rhs)
            {
                return           path == rhs.path
                       && displayName == rhs.displayName
                       && snapShotPath == rhs.snapShotPath;
            }
        };

        extern EngineMeshInfo box;
        extern EngineMeshInfo sphere;
        extern EngineMeshInfo cylinder;
    }

    class AssetSystem;
    class MeshManager
    {
    private:
        AssetSystem* m_assetSystem;

        // asset meta type map. relative to m_allMetaFiles.
        std::unordered_map<std::string, MeshInfo> m_assetMeshMap;

        // key is uuid, value is m_assetMeshMap's key.
        std::unordered_map<std::string, std::string> m_uuidMap;

        // key is name, value is cache meshes.
        std::unordered_map<std::string, std::unique_ptr<Mesh>> m_cacheMeshMap;

        std::vector<Mesh*> m_unReadyMeshes;

        std::vector<VertexIndexType> m_cacheIndicesData = {};
        std::vector<float> m_cacheVerticesData = {};

        // cache merge index buffer.
        VulkanIndexBuffer*  m_indexBuffer  = nullptr;

        // cache merge vertex buffer.
        VulkanVertexBuffer* m_vertexBuffer = nullptr;
    private:
        uint32_t m_lastUploadVertexBufferPos = 0;
        uint32_t m_lastUploadIndexBufferPos = 0;

        void flushAppendBuffer();

    public:
        void addMeshMetaInfo(AssetMeta* newMeta);
        void erasedAssetMetaFile(std::string path);

        // engine mesh never unload.
        Mesh* getEngineMesh(const engineMeshes::EngineMeshInfo& info, bool prepareFallback = true);

        Mesh* getAssetMeshByUUID(const std::string& uuid);
        Mesh* getAssetMeshByMetaName(const std::string& metaName);

        void init(AssetSystem* in);
        void tick();
        void release();

        void bindVertexBuffer(VkCommandBuffer cmd);
        void bindIndexBuffer(VkCommandBuffer cmd);

        const std::unordered_map<std::string, MeshInfo>& getProjectMeshInfos() const { return m_assetMeshMap; }
    };
}