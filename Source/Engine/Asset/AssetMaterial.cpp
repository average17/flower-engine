#include "AssetMaterial.h"
#include <nlohmann/json.hpp>

namespace flower
{
	void MaterialManager::addMaterialFile(AssetMeta* newMeta)
	{
		MaterialInfo materialInfo{};
		readMaterialInfo(newMeta, &materialInfo);

		// update asset type map and meta file map.
		CHECK(!m_materialsMap.contains(materialInfo.filePath));
		m_materialsMap[materialInfo.filePath] = materialInfo;

		CHECK(!m_materialsUUIDMap.contains(materialInfo.UUID));
		m_materialsUUIDMap[materialInfo.UUID] = materialInfo.filePath;
	}

	void MaterialManager::erasedMaterialFile(std::string path)
	{
		if (m_materialsMap.contains(path))
		{
			// erase uuid key
			CHECK(m_materialsUUIDMap.contains(m_materialsMap[path].UUID));
			m_materialsUUIDMap.erase(m_materialsMap[path].UUID);

			// also remove texture info map.
			m_materialsMap.erase(path);
		}
	}

	bool readMaterialInfo(AssetMeta* metaFile,MaterialInfo* inoutInfo)
	{
		nlohmann::json textureMetadata = nlohmann::json::parse(metaFile->json);

		inoutInfo->type = EAssetMaterialType(textureMetadata["type"]);
		inoutInfo->UUID = textureMetadata["UUID"];
		inoutInfo->filePath = textureMetadata["filePath"];

		inoutInfo->bDisneyPBRSet = textureMetadata["bDisneyPBRSet"];
		if(inoutInfo->bDisneyPBRSet)
		{
			std::string materialName = "DisneyPBR_";
			inoutInfo->disneyPBR = std::make_optional<StandardDisneyPBRMaterial>();
			inoutInfo->disneyPBR->baseColorTexture = textureMetadata[materialName + "baseColorTexture"];
			inoutInfo->disneyPBR->normalTexture = textureMetadata[materialName + "normalTexture"];
			inoutInfo->disneyPBR->specularTexture = textureMetadata[materialName + "specularTexture"];
			inoutInfo->disneyPBR->emissiveTexture = textureMetadata[materialName + "emissiveTexture"];
		}

		return true;
	}

	bool writeMaterial(MaterialInfo* info,AssetMeta* inoutMeta)
	{
		inoutMeta->type = (uint32_t)EAssetType::Material;
		inoutMeta->version = FLOWER_ASSET_VERSION;

		nlohmann::json textureMetadata;
		textureMetadata["type"] = (uint32_t)info->type;
		textureMetadata["UUID"] = info->UUID;
		textureMetadata["filePath"] = info->filePath;
		textureMetadata["bDisneyPBRSet"] = info->bDisneyPBRSet;

		if(info->bDisneyPBRSet)
		{
			CHECK(info->disneyPBR.has_value());
			std::string materialName = "DisneyPBR_";

			textureMetadata[materialName + "baseColorTexture"] = info->disneyPBR->baseColorTexture;
			textureMetadata[materialName + "normalTexture"] = info->disneyPBR->normalTexture;
			textureMetadata[materialName + "specularTexture"] = info->disneyPBR->specularTexture;
			textureMetadata[materialName + "emissiveTexture"] = info->disneyPBR->emissiveTexture;
		}

		std::string stringified = textureMetadata.dump();
		inoutMeta->json = stringified;

		inoutMeta->binBlob = {};

		return false;
	}
}