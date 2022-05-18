#pragma once
#include <optional>

#include "../RHI/RHI.h"
#include "AssetCommon.h"

namespace flower
{
	enum class EAssetMaterialType
	{
		Min = 0,

		Standard_DisneyPBR,

		Max,
	};

	struct StandardDisneyPBRMaterial
	{
		// asset store path.
		std::string baseColorTexture;
		std::string normalTexture;
		std::string specularTexture;
		std::string emissiveTexture;
	};

	struct MaterialInfo
	{
		EAssetMaterialType type;
		std::string filePath;
		std::string UUID;

		bool bDisneyPBRSet = false;
		std::optional<StandardDisneyPBRMaterial> disneyPBR;
	};
	extern bool readMaterialInfo(AssetMeta* metaFile, MaterialInfo* inoutInfo);
	extern bool writeMaterial(MaterialInfo* info, AssetMeta* inoutMeta);

	class AssetSystem;
	class MaterialManager
	{
	private:
		AssetSystem* m_assetSystem;

		// key is material path, relative to material files.
		std::unordered_map<std::string, MaterialInfo> m_materialsMap;
		
		// key is uuid, value is material path.
		std::unordered_map<std::string, std::string> m_materialsUUIDMap;

	public:
		void addMaterialFile(AssetMeta* newMeta);
		void erasedMaterialFile(std::string path);

		bool existMaterial(const std::string& uuid) const { return m_materialsUUIDMap.contains(uuid); }

		MaterialInfo* getMaterialInfo(const std::string& uuid) { return &m_materialsMap.at(m_materialsUUIDMap.at(uuid)); }
	};
}