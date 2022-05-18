#pragma once
#include <map>

#include "AssetCommon.h"
#include "../RHI/RHI.h"
#include "../RHI/ObjectUpload.h"

namespace flower
{
	class Texture2DImage;

	constexpr uint32_t SNAPSHOT_SIZE = 64u;

	// load raw texture like .tga, .png, .jpg from file.
	extern Texture2DImage* loadTextureFromFile(
		const std::string& path,
		VkFormat format,
		uint32_t req,
		bool flip,
		bool bGenMipmaps
	);

	enum class ESamplerType : uint32_t
	{
		Min = 0,

		PointClamp,
		PointRepeat,
		LinearClamp,
		LinearRepeat,

		Max,
	};
	extern ESamplerType getDefaultSamplerType();
	extern std::pair<VkSampler, uint32_t> getSamplerAndBindlessIndex(ESamplerType type);

	enum class ETextureFormat : uint32_t
	{
		Min = 0,
		T_R8G8B8A8,
		T_R8G8B8A8_SRGB,
		T_R8G8B8A8_BC3,
		T_R8G8B8A8_SRGB_BC3,

		Max,
	};

	extern bool isBC3Format(ETextureFormat format);
	extern bool isSRGBFormat(ETextureFormat format);
	extern uint32_t getDefaultTextureComponentsCount();
	extern ETextureFormat getDefaultTextureFormat();
	extern VkFormat toVkFormat(ETextureFormat in);
	extern uint32_t getTextureComponentCount(ETextureFormat in);
	extern uint32_t getTotoalBC3PixelCount(uint32_t width,uint32_t height);

	struct TextureInfo
	{
		// texture present format.
		ETextureFormat format;

		// texture cutoff.
		float cutoff;

		uint32_t mipmapLevelsCount;
		bool bCacheMipmap;

		uint32_t width;
		uint32_t height;
		uint32_t depth;

		// texture sampler type.
		ESamplerType samplerType;

		// store compress mode.
		ECompressMode compressMode;

		// texture bin data size in bin file.
		uint32_t binFileDataSize;

		uint32_t rawFileDataSizeInBinFile;

		// meta data bin file size on meta file.
		uint32_t metaBinDataSize;

		// meta file path, for loading.
		std::string metaFilePath;

		// bin file path, for loading.
		std::string binFilePath;

		// UUID of texture.
		std::string UUID;
	};

	// read texture info from meta file.
	extern bool readTextureInfo(AssetMeta* metaFile, TextureInfo* inoutInfo);
	// write data to bin file and meta file, also process some compress situation.
	extern bool writeTexture(TextureInfo* info, void* binFileData,void* metaBinData, AssetMeta* inoutMeta, AssetBin* inoutBin);
	// uncompress texture meta bin file if need.
	extern void uncompressTextureMetaBin(TextureInfo* info,const char* srcBuffer,size_t srcSize,char* dest);
	// uncompress texture bin file if need.
	extern void uncompressTextureBin(TextureInfo* info,const char* srcBuffer,size_t srcSize,char* dest);
	
	extern bool bakeTexture2DLDR(
		const char* pathIn, 
		const char* pathMetaOut,
		const char* pathBinOut,
		ECompressMode compressMode, 
		uint32_t reqComp,
		ETextureFormat format,
		ESamplerType samplerType,
		bool bGenMipmap = false,
		float cutoff = 1.0f
	);

	namespace engineImages
	{
		struct EngineImageInfo
		{
			std::string name;
			VkFormat format;
			uint32_t component = 4;
			bool bHdr = false;
			bool bFlip = false;
			bool bGenMipmap = true;
			ESamplerType samplerType;
		};

		extern EngineImageInfo white; // (1,1,1,1)
		extern EngineImageInfo black; // (0,0,0,1)
		extern EngineImageInfo defaultNormal; // ги0,1,0,1)
		extern EngineImageInfo fallbackCapture;
	}

	// texture manager keep a bindless texture set.
	// all require texture will load by it async,
	class AssetSystem;

	struct CombineTextureBindless
	{
		float cutoff = 1.0f;

		// owner. need release by self.
		Texture2DImage* texture = nullptr;

		// image index in bindless texture set.
		uint32_t textureIndex = 0;

		// sampler index in bindless sampler set.
		uint32_t samplerIndex = 0;

		bool bReady = false;
	};

	class TextureManager
	{
	private:
		AssetSystem* m_assetSystem;

		// asset meta type map. relative to m_allMetaFiles.
		std::unordered_map<std::string, TextureInfo> m_assetTextureMap{};

		// key is uuid, value is m_assetTextureMap's key.
		std::unordered_map<std::string, std::string> m_uuidMap{};

		// key is engine images name, value is image. // never unload.
		std::unordered_map<std::string, std::unique_ptr<CombineTextureBindless>> m_imagesMap;

		// bindless set for textures.
		std::unique_ptr<BindlessTexture> m_bindlessTextures;

	public:
		void addTextureMetaInfo(AssetMeta* newMeta);
		void erasedAssetMetaFile(std::string path);

		CombineTextureBindless* getEngineImage(const engineImages::EngineImageInfo& info);
		
		//loading block.
		CombineTextureBindless* getAssetImageByUUID(const std::string& uuid);

		//loading block.
		CombineTextureBindless* getAssetImageByMetaName(const std::string& uuid);

		//loading block.
		CombineTextureBindless* getAssetImageByUUIDAsync(const std::string& uuid);

		//loading block.
		CombineTextureBindless* getAssetImageByMetaNameAsync(const std::string& uuid);


		BindlessTexture* getBindless() { return m_bindlessTextures.get(); }
		void init(AssetSystem* in);
		void tick();
		void release();
	};
}