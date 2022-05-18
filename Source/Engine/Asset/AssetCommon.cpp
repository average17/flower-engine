#include "AssetCommon.h"
#include "../Core/Core.h"
#include "../Core/String.h"

#include <filesystem>
#include <fstream>

namespace flower
{
	const std::vector<std::string> GSupportRawTextureFormats = {
		".tga",
		".png",
		".psd",
	};

	const std::vector<std::string> GSupportRawMeshFormats = {
		".obj",
	};

	const std::vector<std::string> GSupportAssetFormats = {
		".bin",
		".meta",
	};

	ECompressMode toCompressMode(uint32_t type)
	{
		CHECK(type > (uint32_t)ECompressMode::Min && type < (uint32_t)ECompressMode::Max);
		return ECompressMode(type);
	}

	ECompressMode getDefaultCompressMode()
	{
		return ECompressMode::LZ4;
	}

	EAssetType toAssetFormat(uint32_t type)
	{
		CHECK(type > (uint32_t)EAssetType::Min && type < (uint32_t)EAssetType::Max);
		return EAssetType(type);
	}

	bool saveMetaFile(const char* path,const AssetMeta& inMetaFile)
	{
		std::ofstream outfile;
		outfile.open(path,std::ios::binary | std::ios::out);
		if(!outfile.is_open())
		{
			LOG_IO_FATAL("Fail to write file {0}.",path);
			return false;
		}

		// 0. type
		uint32_t type = inMetaFile.type;
		outfile.write((const char*)&type, sizeof(uint32_t));

		// 1. version
		uint32_t version = inMetaFile.version;
		outfile.write((const char*)&version, sizeof(uint32_t));

		// 2. json lenght
		uint32_t lenght = static_cast<uint32_t>(inMetaFile.json.size());
		outfile.write((const char*)&lenght, sizeof(uint32_t));

		// 3. blob lenght
		uint32_t bloblenght = static_cast<uint32_t>(inMetaFile.binBlob.size());
		outfile.write((const char*)&bloblenght, sizeof(uint32_t));

		// 4. json stream
		outfile.write(inMetaFile.json.data(), lenght);

		// 5. blob stream
		outfile.write(inMetaFile.binBlob.data(), inMetaFile.binBlob.size());

		outfile.close();
		return true;
	}

	bool loadMetaFile(const char* path, AssetMeta& outMetaFile)
	{
		std::ifstream infile;
		infile.open(path,std::ios::binary);
		if(!infile.is_open())
		{
			LOG_IO_ERROR("Fail to open file {0}.",path);
			return false;
		}

		infile.seekg(0);

		// 0. type
		infile.read((char*)&outMetaFile.type, sizeof(uint32_t));

		// 1. version
		infile.read((char*)&outMetaFile.version, sizeof(uint32_t));

		// 2. json lenght
		uint32_t jsonlen = 0;
		infile.read((char*)&jsonlen, sizeof(uint32_t));

		// 3. blob lenght
		uint32_t bloblen = 0;
		infile.read((char*)&bloblen, sizeof(uint32_t));

		// 4. json stream
		outMetaFile.json.resize(jsonlen);
		infile.read(outMetaFile.json.data(),jsonlen);

		// 5. blob stream
		outMetaFile.binBlob.resize(bloblen);
		infile.read(outMetaFile.binBlob.data(),bloblen);

		infile.close();
		return true;
	}

	bool saveBinFile(const char* path, const AssetBin& inBinFile)
	{
		std::ofstream outfile;
		outfile.open(path,std::ios::binary | std::ios::out);
		if(!outfile.is_open())
		{
			LOG_IO_FATAL("Fail to write file {0}.",path);
			return false;
		}

		// 0. blob lenght
		uint32_t bloblenght = static_cast<uint32_t>(inBinFile.binBlob.size());
		outfile.write((const char*)&bloblenght,sizeof(uint32_t));

		// 1. blob stream
		outfile.write(inBinFile.binBlob.data(), inBinFile.binBlob.size());

		outfile.close();
		return true;
	}

	bool loadBinFile(const char* path, AssetBin& outBinFile)
	{
		std::ifstream infile;
		infile.open(path,std::ios::binary);

		if(!infile.is_open())
		{
			LOG_IO_ERROR("Fail to open file {0}.",path);
			return false;
		}

		infile.seekg(0);

		// 0. blob lenght
		uint32_t bloblen = 0;
		infile.read((char*)&bloblen,sizeof(uint32_t));

		// 1. blob stream
		outBinFile.binBlob.resize(bloblen);
		infile.read(outBinFile.binBlob.data(),bloblen);

		infile.close();
		return true;
	}

	bool oneOfEndWithString(const std::string& path, const std::vector<std::string>& list)
	{
		for(auto& format : list)
		{
			if(string::endWith(path,format))
			{
				return true;
			}
		}
		return false;
	}

	bool guessItIsBaseColor(const std::string& path)
	{
		if (path.find("BaseColor") != std::string::npos) return true;
		if (path.find("baseColor") != std::string::npos) return true;

		return false;
	}

	bool getAssetPath(const std::string& rawPath,std::string& outMetaFilePath,std::string& outBinFilePath)
	{
		if(!isSupportFormat(rawPath))
		{
			return false;
		}

		if(oneOfEndWithString(rawPath, {".meta"}))
		{
			outMetaFilePath = rawPath;
			outBinFilePath = string::getFileRawName(rawPath) + ".bin";
			return true;
		}

		if(oneOfEndWithString(rawPath, {".bin"}))
		{
			outMetaFilePath = string::getFileRawName(rawPath) + ".meta";
			outBinFilePath = rawPath;
			return true;
		}

		outMetaFilePath = rawPath + ".meta";
		outBinFilePath = rawPath + ".bin";

		return true;
	}

	bool isProcessedPath(const std::string& path)
	{
		if(oneOfEndWithString(path, GSupportAssetFormats)) return true;
		return false;
	}

	bool isSupportFormat(const std::string& path)
	{
		if(oneOfEndWithString(path, GSupportRawTextureFormats)) return true;
		if(oneOfEndWithString(path, GSupportRawMeshFormats)) return true;
		if(oneOfEndWithString(path, GSupportAssetFormats)) return true;

		return false;
	}

	bool isSupportTextureFormat(const std::string& path)
	{
		if(oneOfEndWithString(path, GSupportRawTextureFormats)) return true;
		return false;
	}

	bool isSupportMeshFormat(const std::string& path)
	{
		if (oneOfEndWithString(path, GSupportRawMeshFormats)) return true;
		return false; 
	}

	bool isHDRTexture(const std::string& path)
	{
		if(string::endWith(path,".exr")) return true;
		return false;
	}

	bool isMetaFile(const std::string& path)
	{
		if(string::endWith(path,".meta")) return true;
		return false;
	}
}