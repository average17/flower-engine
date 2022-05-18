#pragma once
#include "../Component.h"
#include "../../Core/Core.h"
#include "../../Renderer/Material.h"
#include "../../Renderer/Mesh.h"
#include "../../Asset/AssetMesh.h"

namespace flower
{
	
	struct RenderSubMeshProxy
	{
		RenderBounds renderBounds = {};

		uint32_t indexStartPosition = 0;
		uint32_t indexCount = 0;

		GBufferMaterial material;
	};

	struct RenderMeshProxy
	{
		std::vector<RenderSubMeshProxy> subMeshes = {};

		// vertex infos.
		uint32_t vertexStartPosition = 0;
		uint32_t vertexCount = 0;
		uint32_t indexStartPosition = 0;
		uint32_t indexCount = 0;

		glm::mat4 modelMatrix;
		glm::mat4 prevModelMatrix;
	};

	class MeshManager;
	class MaterialManager;
	class TextureManager;

	// simple static mesh component.
	// no instance.
	// when you need to render grass/stone/foliage which is million tons
	// use instance static mesh component please.
	class StaticMeshComponent : public Component
	{
		friend class Scene;

	public:
		StaticMeshComponent() = default;
		COMPONENT_NAME_OVERRIDE(StaticMeshComponent);

		virtual ~StaticMeshComponent() noexcept = default;

	private:
		bool m_bMeshReady = false;
		bool m_bEngineMesh = false;

		std::string m_meshUUID = UNVALID_UUID;
		engineMeshes::EngineMeshInfo m_engineMeshInfo;

		RenderMeshProxy m_cacheProxy;
		MeshManager* m_cacheMeshManager = nullptr;
		MaterialManager* m_cacheMaterialManager = nullptr;
		TextureManager* m_cacheTextureManager = nullptr;

	private:
		void replaceMeshProxy();

	public:
		bool isEngineMesh() const { return m_bEngineMesh; }
		const std::string& getMeshId() const { return m_engineMeshInfo.path; }
		const std::string& getDisplayName() const { return m_engineMeshInfo.displayName; }
		const engineMeshes::EngineMeshInfo& getEngineMeshInfo() const { return m_engineMeshInfo; }

	public:
		bool setMeshId(const engineMeshes::EngineMeshInfo& newId);
		bool setMeshId(const std::string& uuid);

		// collect single draw mesh.
		const RenderMeshProxy& meshCollect();
	};
}