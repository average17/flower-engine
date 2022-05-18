#pragma once
#include <cstdint>
#include <optional>

namespace flower
{
	//!! ensure all shaing model id smaller than 100
	enum class EShadingModel
	{
		UNVALID = 0,
		DisneyBRDF = 1,
	};

	struct GPUMaterialData;
	struct GBufferMaterial
	{
		static GBufferMaterial buildFallback();

		EShadingModel shadingModel = EShadingModel::DisneyBRDF;

		union Param
		{
			struct
			{
				float cutoff;

				uint32_t baseColorId;   
				uint32_t baseColorSampler; 

				uint32_t normalTexId;   
				uint32_t normalSampler;

				uint32_t specularTexId; 
				uint32_t specularSampler;

				uint32_t emissiveTexId; 
				uint32_t emissiveSampler;
			}DisneyBRDF;
		} parameters;

		GPUMaterialData toGPUMaterialData() const;
	};


}