#include "StaticMeshComponent.h"
#include "../SceneNode.h"
#include "../SceneGraph.h"
#include "../../Asset/AssetMesh.h"
#include "../../Asset/AssetSystem.h"
#include "../../Engine.h"

#include "../../Asset/AssetMaterial.h"
#include "../../Asset/AssetTexture.h"

namespace flower
{
	void StaticMeshComponent::replaceMeshProxy()
	{
		// clear cache proxy before replace proxy.
		m_cacheProxy = {};

		if(m_cacheMeshManager == nullptr)
		{
			m_cacheMeshManager = GEngine.getRuntimeModule<AssetSystem>()->getMeshManager();
		}

		if(m_cacheMaterialManager==nullptr)
		{
			m_cacheMaterialManager = GEngine.getRuntimeModule<AssetSystem>()->getMaterialManager();
		}

		if(m_cacheTextureManager==nullptr)
		{
			m_cacheTextureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();
		}

		Mesh* cacheMesh;
		if(m_bEngineMesh)
		{
			cacheMesh = m_cacheMeshManager->getEngineMesh(m_engineMeshInfo);
		}
		else
		{
			if(m_meshUUID == UNVALID_UUID)
			{
				cacheMesh = m_cacheMeshManager->getEngineMesh(engineMeshes::box);
			}
			else
			{
				cacheMesh = m_cacheMeshManager->getAssetMeshByUUID(m_meshUUID);
			}
		}

		m_bMeshReady = cacheMesh->bReady;

		bool bAllTextureReady = true;
		if(Mesh* validMesh = cacheMesh->getValidRenderMesh())
		{
			m_cacheProxy.indexCount = validMesh->indexCount;
			m_cacheProxy.indexStartPosition = validMesh->indexStartPosition;
			m_cacheProxy.vertexCount = validMesh->vertexCount;
			m_cacheProxy.vertexStartPosition = validMesh->vertexStartPosition;

			for(auto& subMesh : validMesh->subMeshes)
			{
				RenderSubMeshProxy subMeshProxy{};
				subMeshProxy.renderBounds = subMesh.renderBounds;
				subMeshProxy.indexCount = subMesh.indexCount;
				subMeshProxy.indexStartPosition = subMesh.indexStartPosition;

				// when replace mesh proxy, we need load src material.
				subMeshProxy.material = GBufferMaterial::buildFallback();

				if(subMesh.material != UNVALID_UUID)
				{
					// default set to brdf.
					subMeshProxy.material.shadingModel = EShadingModel::DisneyBRDF;
					auto* materialInfo = m_cacheMaterialManager->getMaterialInfo(subMesh.material);
					CHECK(materialInfo->disneyPBR.has_value());
					if(materialInfo->disneyPBR->baseColorTexture != UNVALID_UUID)
					{
						auto* basColorInfo = m_cacheTextureManager->getAssetImageByMetaNameAsync(materialInfo->disneyPBR->baseColorTexture);
						
						bAllTextureReady &= basColorInfo->bReady;
						subMeshProxy.material.parameters.DisneyBRDF.cutoff = basColorInfo->cutoff;
						subMeshProxy.material.parameters.DisneyBRDF.baseColorId = basColorInfo->textureIndex;
						subMeshProxy.material.parameters.DisneyBRDF.baseColorSampler = basColorInfo->samplerIndex;
					}
					if(materialInfo->disneyPBR->normalTexture    != UNVALID_UUID)
					{
						auto* normalInfo = m_cacheTextureManager->getAssetImageByMetaNameAsync(materialInfo->disneyPBR->normalTexture);
						
						bAllTextureReady &= normalInfo->bReady;
						subMeshProxy.material.parameters.DisneyBRDF.normalTexId = normalInfo->textureIndex;
						subMeshProxy.material.parameters.DisneyBRDF.normalSampler = normalInfo->samplerIndex;

					}
					if(materialInfo->disneyPBR->specularTexture  != UNVALID_UUID)
					{
						auto* specularInfo = m_cacheTextureManager->getAssetImageByMetaNameAsync(materialInfo->disneyPBR->specularTexture);
						bAllTextureReady &= specularInfo->bReady;

						subMeshProxy.material.parameters.DisneyBRDF.specularTexId = specularInfo->textureIndex;
						subMeshProxy.material.parameters.DisneyBRDF.specularSampler = specularInfo->samplerIndex;
					}
					if(materialInfo->disneyPBR->emissiveTexture  != UNVALID_UUID)
					{
						auto* emissiveInfo = m_cacheTextureManager->getAssetImageByMetaNameAsync(materialInfo->disneyPBR->emissiveTexture);
						bAllTextureReady &= emissiveInfo->bReady;

						subMeshProxy.material.parameters.DisneyBRDF.emissiveTexId = emissiveInfo->textureIndex;
						subMeshProxy.material.parameters.DisneyBRDF.emissiveSampler = emissiveInfo->samplerIndex;
					}
				}
				
				m_cacheProxy.subMeshes.push_back(subMeshProxy);
			}
		}
		m_bMeshReady &= bAllTextureReady;
	}

	bool StaticMeshComponent::setMeshId(const engineMeshes::EngineMeshInfo& newId)
	{
		if(!m_bEngineMesh || newId != m_engineMeshInfo)
		{
			m_engineMeshInfo = newId;
			m_bEngineMesh = true;
			m_bMeshReady = false;
			m_node.lock()->getSceneReference().lock()->setDirty(true);
			return true;
		}

		return false;
	}

	bool StaticMeshComponent::setMeshId(const std::string& uuid)
	{
		if(m_bEngineMesh || m_meshUUID != uuid)
		{
			m_meshUUID = uuid;
			m_bEngineMesh = false;
			m_bMeshReady = false;

			m_node.lock()->getSceneReference().lock()->setDirty(true);
			return true;
		}

		return false;
	}

	const RenderMeshProxy& StaticMeshComponent::meshCollect()
	{
		if(!m_bMeshReady)
		{
			replaceMeshProxy();
		}

		// update mesh model matrix.
		m_cacheProxy.modelMatrix = m_node.lock()->getTransform()->getWorldMatrix();
		m_cacheProxy.prevModelMatrix = m_node.lock()->getTransform()->getPrevWorldMatrix();

		return m_cacheProxy;
	}
}