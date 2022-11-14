#pragma once
#include "AssetCommon.h"
#include "AssetRegistry.h"
#include "AssetSystem.h"
#include "LRUCache.h"
#include "AsyncUploader.h"
#include "../Renderer/MeshMisc.h"

namespace Flower
{
	namespace EngineMeshes
	{
		extern const UUID GBoxUUID;
	}

	class StaticMeshAssetBin;
	class StaticMeshAssetHeader : public AssetHeaderInterface
	{
	private:
		friend class cereal::access;
		std::vector<StaticMeshSubMesh> m_subMeshes = {};
		size_t m_indicesCount;
		size_t m_verticesCount;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::base_class<AssetHeaderInterface>(this));
			archive(m_subMeshes);
			archive(m_indicesCount);
			archive(m_verticesCount);
		}

	public:
		StaticMeshAssetHeader() { }
		StaticMeshAssetHeader(const std::string& name)
			: AssetHeaderInterface(buildUUID(), name)
		{

		}
		virtual EAssetType getType() const { return EAssetType::StaticMesh; }

		const std::vector<StaticMeshSubMesh>& getSubMeshes() const
		{
			return m_subMeshes;
		}

		size_t getVerticesCount() const
		{
			return m_verticesCount;
		}

		size_t getIndicesCount() const
		{
			return m_indicesCount;
		}

		bool initFromRawStaticMesh(const std::filesystem::path& rawPath, std::shared_ptr<RegistryEntry> parentEntry);
	};

	class StaticMeshAssetBin : public AssetBinInterface
	{
	private:
		friend StaticMeshAssetHeader;
		std::vector<StaticMeshVertex> m_vertices;
		std::vector<VertexIndexType> m_indices{ };

	private:
		friend class cereal::access;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(cereal::base_class<AssetBinInterface>(this));
			archive(m_indices);
			archive(m_vertices);
		}

	public:
		StaticMeshAssetBin() { }
		StaticMeshAssetBin(const std::string& name)
			: AssetBinInterface(buildUUID(), name)
		{

		}
		virtual EAssetType getType() const override 
		{ 
			return EAssetType::StaticMesh; 
		}

		const std::vector<uint32_t>& getIndices() const
		{
			return m_indices;
		}

		const std::vector<StaticMeshVertex>& getVertices() const
		{
			return m_vertices;
		}
	};

	class GPUMeshAsset : public LRUAssetInterface
	{
	private:
		std::shared_ptr<VulkanBuffer> m_vertexBuffer = nullptr;
		std::shared_ptr<VulkanBuffer> m_indexBuffer = nullptr;
		VkIndexType m_indexType = VK_INDEX_TYPE_UINT32;

		std::string m_name;

		uint32_t m_singleIndexSize = 0;
		uint32_t m_indexCount = 0;
		uint32_t m_indexCountUint32Count = 0;

		uint32_t m_singleVertexSize = 0;
		uint32_t m_vertexCount = 0;
		uint32_t m_vertexFloat32Count = 0;

		uint32_t m_vertexBufferBindlessIndex = ~0;
		uint32_t m_indexBufferBindlessIndex = ~0;

	public:
		// Immediate build GPU Mesh asset.
		GPUMeshAsset(
			bool bPersistent,
			GPUMeshAsset* fallback,
			const std::string& name,
			VkDeviceSize vertexSize,
			size_t singleVertexSize,
			VkDeviceSize indexSize,
			VkIndexType indexType
		);

		// Lazy buid GPU asset.
		GPUMeshAsset(
			bool bPersistent,
			GPUMeshAsset* fallback,
			const std::string& name
		);

		virtual ~GPUMeshAsset();

		virtual size_t getSize() const override
		{
			return m_vertexBuffer->getSize() + 
			       m_indexBuffer->getSize();
		}

		void prepareToUpload();

		void finishUpload();

		auto& getVertexBuffer()
		{
			return *m_vertexBuffer;
		}

		const auto& getIndicesCount() const
		{
			return m_indexCount;
		}

		const auto& getVerticesCount() const
		{
			return m_vertexCount;
		}

		auto& getIndexBuffer()
		{
			return *m_indexBuffer;
		}

		GPUMeshAsset* getReadyAsset()
		{
			if (isAssetLoading())
			{
				CHECK(m_fallback && "Loading asset must exist one fallback.");
				return dynamic_cast<GPUMeshAsset*>(m_fallback);
			}
			return this;
		}

		uint32_t getIndicesBindlessIndex()
		{
			return getReadyAsset()->m_indexBufferBindlessIndex;
		}

		uint32_t getVerticesBindlessIndex()
		{
			return getReadyAsset()->m_vertexBufferBindlessIndex;
		}
	};

	class MeshContext
	{
	private:
		std::unique_ptr<LRUAssetCache<GPUMeshAsset>> m_lruCache;
		std::unique_ptr<BindlessStorageBuffer> m_vertexBindlessBuffer;
		std::unique_ptr<BindlessStorageBuffer> m_indexBindlessBuffer;

	public:
		MeshContext() = default;

		void init();
		void release();

		BindlessStorageBuffer* getBindlessIndexBuffers() const
		{
			return m_indexBindlessBuffer.get();
		}

		BindlessStorageBuffer* getBindlessVertexBuffers() const
		{
			return m_vertexBindlessBuffer.get();
		}

		bool isAssetExist(const UUID& id)
		{
			return m_lruCache->contain(id);
		}

		void insertGPUAsset(const UUID& uuid, std::shared_ptr<GPUMeshAsset> mesh)
		{
			m_lruCache->insert(uuid, mesh);
		}

		std::shared_ptr<GPUMeshAsset> getMesh(const UUID& id)
		{
			return m_lruCache->tryGet(id);
		}

		std::shared_ptr<GPUMeshAsset> getOrCreateLRUMesh(const AssetHeaderUUID& id);
		std::shared_ptr<GPUMeshAsset> getOrCreateLRUMesh(std::shared_ptr<StaticMeshAssetHeader> header);
	};

	using MeshManager = Singleton<MeshContext>;

	struct AssetMeshLoadTask : public AssetLoadTask
	{
		AssetMeshLoadTask() { }

		// Working mesh.
		std::shared_ptr<GPUMeshAsset> meshAssetGPU = nullptr;

		virtual uint32_t uploadSize() const override
		{
			return uint32_t(meshAssetGPU->getSize());
		}
	};

	struct StaticMeshRawDataLoadTask : public AssetMeshLoadTask
	{
		StaticMeshRawDataLoadTask() { }

		std::vector<uint8_t> cacheVertexData;
		std::vector<uint8_t> cacheIndexData;

		virtual void finishCallback() override;

		virtual void uploadFunction(
			uint32_t stageBufferOffset, 
			RHICommandBufferBase& commandBuffer,
			VulkanBuffer& stageBuffer) override;

		// Build from raw data.
		static std::shared_ptr<StaticMeshRawDataLoadTask> buildFromData(
			const std::string& name,
			const UUID& uuid,
			bool bPersistent,
			uint8_t* indices,
			size_t indexSize,
			VkIndexType indexType,
			uint8_t* vertices,
			size_t vertexSize,
			size_t singleVertexSize
		);
	};

	struct StaticMeshLoadTask : public AssetMeshLoadTask
	{
		StaticMeshLoadTask() { }

		std::shared_ptr<StaticMeshAssetHeader> cacheHeader;

		virtual void finishCallback() override;
		virtual void uploadFunction(
			uint32_t stageBufferOffset,
			RHICommandBufferBase& commandBuffer,
			VulkanBuffer& stageBuffer) override;

		// Build from asset registry
		static std::shared_ptr<StaticMeshLoadTask> build(
			std::shared_ptr<RegistryEntry> registry,
			bool bPersistent
		);
	};

}

CEREAL_REGISTER_TYPE(Flower::StaticMeshAssetHeader);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Flower::AssetHeaderInterface, Flower::StaticMeshAssetHeader)

CEREAL_REGISTER_TYPE(Flower::StaticMeshAssetBin);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Flower::AssetBinInterface, Flower::StaticMeshAssetBin)
