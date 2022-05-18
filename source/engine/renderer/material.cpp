#include "Material.h"
#include "../Asset/AssetSystem.h"
#include "../Engine.h"
#include "../Asset/AssetMesh.h"
#include "../Asset/AssetMaterial.h"
#include "../Asset/AssetTexture.h"
#include "DeferredRenderer/SceneStaticUniform.h"

namespace flower
{
	GBufferMaterial flower::GBufferMaterial::buildFallback()
	{
		GBufferMaterial result {};

		auto* textureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();

		result.shadingModel = EShadingModel::DisneyBRDF;
		{
			auto* whiteTex =  textureManager->getEngineImage(engineImages::white);

			result.parameters.DisneyBRDF.cutoff = 1.0f;
			result.parameters.DisneyBRDF.baseColorId = whiteTex->textureIndex;
			result.parameters.DisneyBRDF.baseColorSampler = whiteTex->samplerIndex;
		}

		{
			auto* normalInfo = textureManager->getEngineImage(engineImages::defaultNormal);
			result.parameters.DisneyBRDF.normalTexId = normalInfo->textureIndex;
			result.parameters.DisneyBRDF.normalSampler = normalInfo->samplerIndex;

		}

		{
			auto* specularInfo = textureManager->getEngineImage(engineImages::white);
			result.parameters.DisneyBRDF.specularTexId = specularInfo->textureIndex;
			result.parameters.DisneyBRDF.specularSampler = specularInfo->samplerIndex;
		}

		{
			auto* emissiveInfo = textureManager->getEngineImage(engineImages::black);

			result.parameters.DisneyBRDF.emissiveTexId = emissiveInfo->textureIndex;
			result.parameters.DisneyBRDF.emissiveSampler = emissiveInfo->samplerIndex;
		}
		

		return result;
	}

	GPUMaterialData GBufferMaterial::toGPUMaterialData() const
	{
		GPUMaterialData result {};

		result.shadingModel = (uint32_t)this->shadingModel;
		result.cutoff = this->parameters.DisneyBRDF.cutoff;
		result.baseColorId  = this->parameters.DisneyBRDF.baseColorId;
		result.baseColorSampler  = this->parameters.DisneyBRDF.baseColorSampler;
		result.normalSampler  = this->parameters.DisneyBRDF.normalSampler;
		result.normalTexId  = this->parameters.DisneyBRDF.normalTexId;
		result.emissiveSampler  = this->parameters.DisneyBRDF.emissiveSampler;
		result.emissiveTexId  = this->parameters.DisneyBRDF.emissiveTexId;
		result.specularSampler  = this->parameters.DisneyBRDF.specularSampler;
		result.specularTexId  = this->parameters.DisneyBRDF.specularTexId;

		return result;
	}

}

