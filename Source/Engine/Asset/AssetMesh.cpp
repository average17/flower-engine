#include "AssetMesh.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <nlohmann/json.hpp>
#include <lz4/lz4.h>
#include <filesystem>
#include <execution>

#include "../Core/UUID.h"
#include "../UI/UISystem.h"
#include "../UI/Icon.h"
#include "../Core/UUID.h"
#include "../RHI/RHI.h"
#include "AssetSystem.h"
#include "AssetMaterial.h"
#include "../Core/String.h"

namespace flower
{
	static AutoCVarInt32 cVarBaseVRamForVertexBuffer(
		"r.Vram.BaseVertexBuffer",
		"Persistent vram for vertex buffer(mb).",
		"Vram",
		128,
		CVarFlags::InitOnce | CVarFlags::ReadOnly
	);

	static AutoCVarInt32 cVarBaseVRamForIndexBuffer(
		"r.Vram.BaseIndexBuffer",
		"Persistent vram for index buffer(mb).",
		"Vram",
		64,
		CVarFlags::InitOnce | CVarFlags::ReadOnly
	);

	static AutoCVarInt32 cVarVertexBufferIncrementSize(
		"r.Vram.VertexBufferIncrementSize",
		"Increment vram size for vertex buffer(mb) when base vertex buffer no fit.",
		"Vram",
		16,
		CVarFlags::InitOnce | CVarFlags::ReadOnly
	);

	static AutoCVarInt32 cVarIndexBufferIncrementSize(
		"r.Vram.IndexBufferIncrementSize",
		"Increment vram size for index buffer(mb) when base index buffer no fit.",
		"Vram",
		8,
		CVarFlags::InitOnce | CVarFlags::ReadOnly
	);

	engineMeshes::EngineMeshInfo engineMeshes::box = { .path = "Mesh/Box.obj", .displayName = "Box", .snapShotPath = "Mesh/BoxSnapshot.png"};
	engineMeshes::EngineMeshInfo engineMeshes::sphere = { .path = "Mesh/Sphere.obj", .displayName = "Sphere", .snapShotPath = "Mesh/SphereSnapshot.png"};
	engineMeshes::EngineMeshInfo engineMeshes::cylinder = { .path = "Mesh/Cylinder.obj", .displayName = "Cylinder", .snapShotPath = "Mesh/CylinderSnapshot.png"};

	int32_t getSize(const std::vector<EVertexAttribute>& layouts)
	{
		int32_t size = 0;
		for (const auto& layout : layouts)
		{
			int32_t layoutSize = getVertexAttributeSize(layout);
			size += layoutSize;
		}

		return size;
	}

	int32_t getCount(const std::vector<EVertexAttribute>& layouts)
	{
		int32_t size = 0;
		for (const auto& layout : layouts)
		{
			int32_t layoutSize = getVertexAttributeElementCount(layout);
			size += layoutSize;
		}

		return size;
	}

	std::string toString(const std::vector<EVertexAttribute>& layouts)
	{
		std::string keyStr = "";
		for (const auto& layout : layouts)
		{
			uint32_t key = (uint32_t)layout;
			keyStr += std::to_string(key);
		}

		return keyStr;
	}

	std::vector<EVertexAttribute> toVertexAttributes(std::string keyStr)
	{
		std::vector<EVertexAttribute> res = {};
		for (auto& charKey : keyStr)
		{
			uint32_t key = uint32_t(charKey - '0');
			CHECK(key > (uint32_t)EVertexAttribute::none && key < (uint32_t)EVertexAttribute::count);
			EVertexAttribute attri = (EVertexAttribute)key;
			res.push_back(attri);
		}
		return res;
	}

	bool readMeshInfo(MeshInfo& info, AssetMeta* file)
	{
		if (file->type != (uint32_t)EAssetType::Mesh)
		{
			return false;
		}
		
		nlohmann::json metadata = nlohmann::json::parse(file->json);

		info.vertCount    = metadata["vertCount"];
		info.subMeshCount = metadata["subMeshCount"];
		info.originalFile = metadata["originalFile"];
		info.UUID         = metadata["UUID"];
		info.compressMode = (ECompressMode)metadata["compressMode"];
		info.attributeLayout = toVertexAttributes(metadata["vertexAttributes"]);

		info.subMeshInfos.resize(info.subMeshCount);

		for (uint32_t i = 0; i < info.subMeshCount; i++)
		{
			auto& subMeshInfo = info.subMeshInfos[i];
			std::string subMeshPreStr = "subMesh";
			subMeshPreStr += std::to_string(i);

			subMeshInfo.materialPath = metadata[subMeshPreStr + "MaterialPath"];
			subMeshInfo.indexCount   = metadata[subMeshPreStr + "IndexCount"];
			subMeshInfo.vertexCount  = metadata[subMeshPreStr + "VertexCount"];
			subMeshInfo.indexStartPosition = metadata[subMeshPreStr + "IndexStartPosition"];

			std::vector<float> subBoundsData;
			subBoundsData.reserve(7);
			subBoundsData = metadata[subMeshPreStr + "Bounds"].get<std::vector<float>>();

			subMeshInfo.bounds.origin[0]  = subBoundsData[0];
			subMeshInfo.bounds.origin[1]  = subBoundsData[1];
			subMeshInfo.bounds.origin[2]  = subBoundsData[2];
			subMeshInfo.bounds.radius     = subBoundsData[3];
			subMeshInfo.bounds.extents[0] = subBoundsData[4];
			subMeshInfo.bounds.extents[1] = subBoundsData[5];
			subMeshInfo.bounds.extents[2] = subBoundsData[6];
		}
		return true;
	}

	bool writeMesh(MeshInfo* info, AssetMeta* metaFile, AssetBin* binFile, char* vertexData, char* indexData, bool bCompress)
	{
		metaFile->version = FLOWER_ASSET_VERSION;
		metaFile->type = (uint32_t)EAssetType::Mesh;

		// meta data start.
		nlohmann::json meshMetadata;
		meshMetadata["subMeshCount"] = info->subMeshCount;
		meshMetadata["vertexAttributes"] = toString(info->attributeLayout);

		// submesh start.
		meshMetadata["vertCount"] = info->vertCount;
		uint32_t indicesNum = 0;
		for (uint32_t i = 0; i < info->subMeshCount; i++)
		{
			auto& subMeshInfo = info->subMeshInfos[i];
			std::string subMeshPreStr = "subMesh";
			subMeshPreStr += std::to_string(i);

			meshMetadata[subMeshPreStr + "MaterialPath"] = subMeshInfo.materialPath;
			meshMetadata[subMeshPreStr + "IndexCount"]   = subMeshInfo.indexCount;
			meshMetadata[subMeshPreStr + "VertexCount"]  = subMeshInfo.vertexCount;
			meshMetadata[subMeshPreStr + "IndexStartPosition"] = subMeshInfo.indexStartPosition;

			indicesNum += subMeshInfo.indexCount;

			std::vector<float> subBoundsData;
			subBoundsData.resize(7);
			subBoundsData[0] = subMeshInfo.bounds.origin[0];
			subBoundsData[1] = subMeshInfo.bounds.origin[1];
			subBoundsData[2] = subMeshInfo.bounds.origin[2];
			subBoundsData[3] = subMeshInfo.bounds.radius;
			subBoundsData[4] = subMeshInfo.bounds.extents[0];
			subBoundsData[5] = subMeshInfo.bounds.extents[1];
			subBoundsData[6] = subMeshInfo.bounds.extents[2];

			meshMetadata[subMeshPreStr + "Bounds"] = subBoundsData;
		}

		meshMetadata["originalFile"] = info->originalFile;
		meshMetadata["UUID"] = info->UUID;

		uint32_t vertBufferSize = info->vertCount * getSize(info->attributeLayout);
		uint32_t indicesBufferSize = indicesNum * sizeof(VertexIndexType);
		uint32_t fullSize = vertBufferSize + indicesBufferSize;

		std::vector<char> mergedBuffer;
		mergedBuffer.resize(fullSize);

		// mereged vertex buffer and index buffer together.
		memcpy(mergedBuffer.data(), vertexData, vertBufferSize);
		memcpy(mergedBuffer.data() + vertBufferSize, indexData, indicesBufferSize);

		if (bCompress)
		{
			size_t compressStaging = LZ4_compressBound(static_cast<int>(fullSize));
			binFile->binBlob.resize(compressStaging);
			int compressedSize = LZ4_compress_default(
				mergedBuffer.data(),
				binFile->binBlob.data(),
				static_cast<int>(mergedBuffer.size()),
				static_cast<int>(compressStaging)
			);

			binFile->binBlob.resize(compressedSize);
			meshMetadata["compressMode"] = (uint32_t)ECompressMode::LZ4;
		}
		else
		{
			binFile->binBlob.resize(fullSize);
			memcpy(binFile->binBlob.data(), (const char*)mergedBuffer.data(), fullSize);
			meshMetadata["compressMode"] = (uint32_t)ECompressMode::None;
		}

		metaFile->json = meshMetadata.dump();
		metaFile->binBlob = {};
		return true;
	}

	void unpackMeshBinFile(MeshInfo* info, const char* sourcebuffer, size_t sourceSize, std::vector<float>& vertexBufer, std::vector<VertexIndexType>& indexBuffer)
	{
		uint32_t indicesCount = 0;
		for (auto& subInfo : info->subMeshInfos)
		{
			indicesCount += subInfo.indexCount;
		}
		indexBuffer.resize(indicesCount);
		vertexBufer.resize(info->vertCount * getCount(info->attributeLayout));

		uint32_t vertBufferSize = info->vertCount * getSize(info->attributeLayout);
		uint32_t indicesBufferSize = indicesCount * sizeof(VertexIndexType);

		std::vector<char> packBuffer;
		packBuffer.resize(vertBufferSize + indicesBufferSize);

		if (info->compressMode == ECompressMode::LZ4)
		{
			LZ4_decompress_safe(
				sourcebuffer,
				packBuffer.data(),
				static_cast<int>(sourceSize),
				static_cast<int>(packBuffer.size())
			);
		}
		else
		{
			memcpy(packBuffer.data(), sourcebuffer, sourceSize);
		}

		memcpy((char*)vertexBufer.data(), packBuffer.data(), vertBufferSize);
		memcpy((char*)indexBuffer.data(), packBuffer.data() + vertBufferSize, indicesBufferSize);
	}

	struct AssimpModelProcess
	{
		std::vector<MeshInfo::SubMeshInfo> m_subMeshInfos{};
		std::vector<StandardVertex> m_vertices{};
		std::vector<VertexIndexType> m_indices{};

		// load material texture.
		std::vector<std::string> loadMaterialTexture(aiMaterial* mat, aiTextureType type)
		{
			std::vector<std::string> textures;
			for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
			{
				aiString str;
				mat->GetTexture(type, i, &str);
				textures.push_back(str.C_Str());
			}
			return textures;
		}

		MeshInfo::SubMeshInfo processMesh(aiMesh* mesh, const aiScene* scene, const std::string& materialFolderPath)
		{
			MeshInfo::SubMeshInfo subMeshInfo{};
			subMeshInfo.indexStartPosition = (uint32_t)m_indices.size();
			uint32_t indexOffset = (uint32_t)m_vertices.size();

			std::vector<StandardVertex> vertices{};
			std::vector<VertexIndexType> indices{};

			for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			{
				StandardVertex vertex;

				glm::vec3 vector{};
				vector.x = mesh->mVertices[i].x;
				vector.y = mesh->mVertices[i].y;
				vector.z = mesh->mVertices[i].z;
				vertex.pos = vector;

				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.normal = vector;

				if (mesh->mTextureCoords[0])
				{
					glm::vec2 vec{};
					vec.x = mesh->mTextureCoords[0][i].x;
					vec.y = mesh->mTextureCoords[0][i].y;
					vertex.uv0 = vec;
				}
				else
				{
					vertex.uv0 = glm::vec2(0.0f, 0.0f);
				}

				glm::vec4 tangentVec{};
				tangentVec.x = mesh->mTangents[i].x;
				tangentVec.y = mesh->mTangents[i].y;
				tangentVec.z = mesh->mTangents[i].z;

				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;

				glm::vec3 bitangent{};
				bitangent.x = mesh->mBitangents[i].x;
				bitangent.y = mesh->mBitangents[i].y;
				bitangent.z = mesh->mBitangents[i].z;

				// tangent sign process.
				tangentVec.w = glm::sign(glm::dot(glm::normalize(bitangent), glm::normalize(glm::cross(vertex.normal, vector))));
				vertex.tangent = tangentVec;
				vertices.push_back(vertex);
			}

			for (unsigned int i = 0; i < mesh->mNumFaces; i++)
			{
				aiFace face = mesh->mFaces[i];
				for (unsigned int j = 0; j < face.mNumIndices; j++)
				{
					indices.push_back(indexOffset + face.mIndices[j]);
				}
			}

			m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());
			m_indices.insert(m_indices.end(), indices.begin(), indices.end());

			subMeshInfo.indexCount = (uint32_t)indices.size();
			subMeshInfo.vertexCount = (uint32_t)vertices.size();

			// aabb bounds process.
			auto aabbMax = mesh->mAABB.mMax;
			auto aabbMin = mesh->mAABB.mMin;
			auto aabbExt = (aabbMax - aabbMin) * 0.5f;
			auto aabbCenter = aabbExt + aabbMin;
			subMeshInfo.bounds.extents[0] = aabbExt.x;
			subMeshInfo.bounds.extents[1] = aabbExt.y;
			subMeshInfo.bounds.extents[2] = aabbExt.z;
			subMeshInfo.bounds.origin[0]  = aabbCenter.x;
			subMeshInfo.bounds.origin[1]  = aabbCenter.y;
			subMeshInfo.bounds.origin[2]  = aabbCenter.z;
			subMeshInfo.bounds.radius = glm::distance(
				glm::vec3(aabbMax.x, aabbMax.y, aabbMax.z),
				glm::vec3(aabbCenter.x, aabbCenter.y, aabbCenter.z)
			);

			if (std::filesystem::exists(materialFolderPath))
			{
				// standard pbr texture prepare.
				std::vector<std::string> baseColorTextures{};
				std::vector<std::string> normalTextures{};
				std::vector<std::string> specularTextures{};
				std::vector<std::string> emissiveTextures{};
				if (mesh->mMaterialIndex >= 0)
				{
					aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

					baseColorTextures = loadMaterialTexture(material, aiTextureType_DIFFUSE);
					CHECK(baseColorTextures.size() <= 1);

					normalTextures = loadMaterialTexture(material, aiTextureType_HEIGHT);
					CHECK(normalTextures.size() <= 1);

					specularTextures = loadMaterialTexture(material, aiTextureType_SPECULAR);
					CHECK(specularTextures.size() <= 1);

					emissiveTextures = loadMaterialTexture(material, aiTextureType_EMISSIVE);
					CHECK(emissiveTextures.size() <= 1);

					std::string materialPath = materialFolderPath + material->GetName().C_Str() + ".meta";

					if(std::filesystem::exists(materialPath))
					{
						MaterialInfo materialInfo{};
						AssetMeta tempMetaFile{};
						CHECK(loadMetaFile(materialPath.c_str(),tempMetaFile));
						readMaterialInfo(&tempMetaFile, &materialInfo);
						subMeshInfo.materialPath = materialInfo.UUID;
					}
					else
					{
						MaterialInfo info{};
						info.bDisneyPBRSet = true;
						info.filePath = materialPath;
						info.type = EAssetMaterialType::Standard_DisneyPBR;
						info.UUID = getUUID();

						info.disneyPBR = std::make_optional<StandardDisneyPBRMaterial>();


						if(baseColorTextures.size() > 0) info.disneyPBR->baseColorTexture = std::filesystem::absolute(baseColorTextures[0]).string() + ".meta";
						if(normalTextures.size() > 0) info.disneyPBR->normalTexture =  std::filesystem::absolute(normalTextures[0]).string() + ".meta";
						if(specularTextures.size() > 0) info.disneyPBR->specularTexture =  std::filesystem::absolute(specularTextures[0]).string() + ".meta";
						if(emissiveTextures.size() > 0) info.disneyPBR->emissiveTexture =  std::filesystem::absolute(emissiveTextures[0]).string() + ".meta";
						subMeshInfo.materialPath = info.UUID;

						AssetMeta materialMetaFile{};
						writeMaterial(&info,&materialMetaFile);
						saveMetaFile(materialPath.c_str(), materialMetaFile);
					}
					
				}
				else // no material found, keep empty.
				{
					subMeshInfo.materialPath = UNVALID_UUID;
				}
			}
			else
			{
				subMeshInfo.materialPath = UNVALID_UUID;
			}

			return subMeshInfo;
		}

		void processNode(aiNode* node, const aiScene* scene, const std::string& materialFolderPath = "")
		{
			for (unsigned int i = 0; i < node->mNumMeshes; i++)
			{
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				m_subMeshInfos.push_back(processMesh(mesh, scene, materialFolderPath));
			}

			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				processNode(node->mChildren[i], scene, materialFolderPath);
			}
		}
	};

	bool bakeAssimpMesh(const char* pathIn, const char* metaPathOut, const char* binPathOut, bool bCompress)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(pathIn, aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOG_ERROR("ERROR::ASSIMP::{0}", importer.GetErrorString());
			return false;
		}

		std::string path = pathIn;
		std::string mtlSearchPath;
		std::string meshName;
		size_t pos = path.find_last_of("/\\");
		if (pos != std::string::npos)
		{
			mtlSearchPath = path.substr(0, pos);
			meshName = path.substr(pos);
		}

		// auto create material folder path if no exist.
		mtlSearchPath += "/";
		std::string materialFolderPath = mtlSearchPath + "materials/";
		if (!std::filesystem::exists(materialFolderPath))
		{
			std::filesystem::create_directory(materialFolderPath);
		}

		MeshInfo info = {};
		info.subMeshCount = (uint32_t)scene->mNumMeshes;
		info.originalFile = pathIn;

		// prepare mesh uuid when bake to asset.
		info.UUID = getUUID();
		info.compressMode = bCompress ? ECompressMode::LZ4 : ECompressMode::None;
		info.attributeLayout = getStandardMeshAttributes();

		AssimpModelProcess processor{};
		processor.processNode(scene->mRootNode, scene, materialFolderPath);
		info.subMeshInfos = processor.m_subMeshInfos;
		info.vertCount = (uint32_t)processor.m_vertices.size();

		std::vector<float> vertices{};
		vertices.reserve(processor.m_vertices.size() * getCount(getStandardMeshAttributes()));
		for (const auto& vertex : processor.m_vertices)
		{
			vertices.push_back(vertex.pos.x);
			vertices.push_back(vertex.pos.y);
			vertices.push_back(vertex.pos.z);
			vertices.push_back(vertex.uv0.x);
			vertices.push_back(vertex.uv0.y);
			vertices.push_back(vertex.normal.x);
			vertices.push_back(vertex.normal.y);
			vertices.push_back(vertex.normal.z);
			vertices.push_back(vertex.tangent.x);
			vertices.push_back(vertex.tangent.y);
			vertices.push_back(vertex.tangent.z);
			vertices.push_back(vertex.tangent.w);
		}

		AssetMeta metaFile{ };
		AssetBin binFile{ };
		bool res = writeMesh(
			&info,
			&metaFile,
			&binFile,
			(char*)vertices.data(),
			(char*)processor.m_indices.data(),
			bCompress
		);

		if (!res)
		{
			LOG_IO_ERROR("Fail to write mesh {0} bin or meta file.", path);
		}

		std::string pathMtl = string::getFileRawName(pathIn) + ".mtl";
		if(std::filesystem::exists(pathMtl))
		{
			std::filesystem::remove(pathMtl);
		}

		return saveBinFile(binPathOut, binFile) && saveMetaFile(metaPathOut, metaFile);
	}

	void MeshManager::tick()
	{
		flushAppendBuffer();
	}

	void MeshManager::flushAppendBuffer()
	{
		VkDeviceSize incrementVertexBufferSize = static_cast<VkDeviceSize>(cVarVertexBufferIncrementSize.get()) * 1024 * 1024;
		VkDeviceSize incrementIndexBufferSize  = static_cast<VkDeviceSize>(cVarIndexBufferIncrementSize.get())  * 1024 * 1024;

		if(m_lastUploadVertexBufferPos != (uint32_t)m_cacheVerticesData.size())
		{
			uint32_t upLoadStartPos = m_lastUploadVertexBufferPos;
			uint32_t upLoadEndPos = (uint32_t)m_cacheVerticesData.size();

			uint64_t perElemtSize = sizeof(m_cacheVerticesData[0]);
			m_lastUploadVertexBufferPos = upLoadEndPos;

			VkDeviceSize currentVertexDataDeviceSize = VkDeviceSize(perElemtSize * m_cacheVerticesData.size());
			VkDeviceSize lastDeviceSize = m_vertexBuffer->getVulkanBuffer()->getSize();

			// if out of range we need reallocate a bigger buffer and copy flush.
			if(currentVertexDataDeviceSize >= lastDeviceSize)
			{
				VkDeviceSize newDeviceSize = ((currentVertexDataDeviceSize / incrementVertexBufferSize) + 2) * incrementVertexBufferSize;

				LOG_INFO("Reallocate {0} mb local vram for vertex buffer!", newDeviceSize / (1024 * 1024));

				vkQueueWaitIdle(RHI::get()->getVulkanDevice()->graphicsQueue);
				delete m_vertexBuffer;

				m_vertexBuffer = VulkanVertexBuffer::create(
					RHI::get()->getVulkanDevice(),
					RHI::get()->getGraphicsCommandPool(),
					newDeviceSize,
					true,
					true 
				);

				// reset upload start pos.
				upLoadStartPos = 0;
			}

			CHECK(upLoadEndPos > upLoadStartPos);

			VkDeviceSize uploadDeviceSize = VkDeviceSize(perElemtSize * (upLoadEndPos - upLoadStartPos));
			VkDeviceSize offsetDeviceSize = VkDeviceSize(perElemtSize * upLoadStartPos);

			VulkanBuffer* stageBuffer = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uploadDeviceSize,
				(void *)(m_cacheVerticesData.data() + upLoadStartPos)
			);

			m_vertexBuffer->getVulkanBuffer()->stageCopyFrom(
				*stageBuffer, uploadDeviceSize,
				RHI::get()->getVulkanDevice()->graphicsQueue, 0, offsetDeviceSize);
			delete stageBuffer;
		}

		if(m_lastUploadIndexBufferPos != (uint32_t)m_cacheIndicesData.size())
		{
			uint32_t upLoadStartPos = m_lastUploadIndexBufferPos;
			uint32_t upLoadEndPos = (uint32_t)m_cacheIndicesData.size();

			m_lastUploadIndexBufferPos = upLoadEndPos;
			uint64_t perElemtSize = sizeof(m_cacheIndicesData[0]);

			VkDeviceSize currentIndexDataDeviceSize = VkDeviceSize(perElemtSize * m_cacheIndicesData.size());
			VkDeviceSize lastDeviceSize = m_indexBuffer->getVulkanBuffer()->getSize();

			// same process like vertex buffer.
			if(currentIndexDataDeviceSize >= lastDeviceSize)
			{
				VkDeviceSize newDeviceSize = ((currentIndexDataDeviceSize / incrementIndexBufferSize) + 2) * incrementIndexBufferSize;
				delete m_indexBuffer;

				LOG_INFO("Reallocate {0} mb local vram for index buffer!", newDeviceSize / (1024 * 1024));
				vkQueueWaitIdle(RHI::get()->getVulkanDevice()->graphicsQueue);

				m_indexBuffer = VulkanIndexBuffer::create(
					RHI::get()->getVulkanDevice(),
					newDeviceSize,
					RHI::get()->getGraphicsCommandPool(),
					true,
					true
				);

				// all need reupload.
				upLoadStartPos = 0;
			}

			CHECK(upLoadEndPos > upLoadStartPos);
			VkDeviceSize uploadDeviceSize = VkDeviceSize(perElemtSize * (upLoadEndPos - upLoadStartPos));
			VkDeviceSize offsetDeviceSize = VkDeviceSize(perElemtSize * upLoadStartPos);

			VulkanBuffer* stageBuffer = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				uploadDeviceSize,
				(void *)(m_cacheIndicesData.data() + upLoadStartPos)
			);

			m_indexBuffer->getVulkanBuffer()->stageCopyFrom(
				*stageBuffer, 
				uploadDeviceSize,
				RHI::get()->getVulkanDevice()->graphicsQueue, 
				0, 
				offsetDeviceSize);

			delete stageBuffer;
		}

		std::for_each(std::execution::par_unseq, m_unReadyMeshes.begin(), m_unReadyMeshes.end(), [](Mesh* mesh)
		{
			mesh->bReady = true;
		});
		m_unReadyMeshes.clear();
	}

	void MeshManager::addMeshMetaInfo(AssetMeta* newMeta)
	{
		MeshInfo meshInfo{};
		readMeshInfo(meshInfo, newMeta);

		// update asset type map and meta file map.
		CHECK(!m_assetMeshMap.contains(meshInfo.originalFile));
		m_assetMeshMap[meshInfo.originalFile] = meshInfo;

		CHECK(!m_uuidMap.contains(meshInfo.UUID));
		m_uuidMap[meshInfo.UUID] = meshInfo.originalFile;
	}

	void MeshManager::erasedAssetMetaFile(std::string path)
	{
		if (m_assetMeshMap.contains(path))
		{
			// erase uuid key
			CHECK(m_uuidMap.contains(m_assetMeshMap[path].UUID));
			m_uuidMap.erase(m_assetMeshMap[path].UUID);

			// also remove texture info map.
			m_assetMeshMap.erase(path);
		}
	}

	Mesh* MeshManager::getEngineMesh(const engineMeshes::EngineMeshInfo& info, bool prepareFallback)
	{
		if (m_cacheMeshMap.contains(info.path))
		{
			return m_cacheMeshMap[info.path].get();
		}

		CHECK(std::filesystem::exists(info.path));

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(info.path, aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			LOG_ERROR("ERROR::ASSIMP::{0}", importer.GetErrorString());
			return nullptr;
		}

		auto newMesh = std::make_unique<Mesh>();
		m_unReadyMeshes.push_back(newMesh.get());

		newMesh->bReady = false;
		newMesh->layout = getStandardMeshAttributes();

		AssimpModelProcess processor{};
		processor.processNode(scene->mRootNode, scene);

		uint32_t lastMeshVerticesPos = (uint32_t)m_cacheVerticesData.size();
		uint32_t lastMeshIndexPos = (uint32_t)m_cacheIndicesData.size();

		newMesh->vertexStartPosition = lastMeshVerticesPos;
		newMesh->vertexCount = (uint32_t)processor.m_vertices.size();
		newMesh->indexStartPosition = lastMeshIndexPos;
		newMesh->indexCount = (uint32_t)processor.m_indices.size();

		// prepare submesh info.
		for (const auto& subMeshInfo : processor.m_subMeshInfos)
		{
			SubMesh subMesh = {};
			subMesh.renderBounds.radius = subMeshInfo.bounds.radius;

			subMesh.renderBounds.extents = { 
				subMeshInfo.bounds.extents[0],
				subMeshInfo.bounds.extents[1],
				subMeshInfo.bounds.extents[2]
			};

			subMesh.renderBounds.origin = {
				subMeshInfo.bounds.origin[0],
				subMeshInfo.bounds.origin[1],
				subMeshInfo.bounds.origin[2]
			};

			// material default set to unvalid.
			subMesh.material = UNVALID_UUID;
			subMesh.indexCount = subMeshInfo.indexCount;

			// submesh index start position is meshlib's index buffer index.
			subMesh.indexStartPosition = subMeshInfo.indexStartPosition + lastMeshIndexPos;

			newMesh->subMeshes.push_back(subMesh);
		}

		// upload index buffer and vertex buffer.
		std::vector<float> vertices{};
		vertices.reserve(processor.m_vertices.size() * getCount(getStandardMeshAttributes()));
		for (const auto& vertex : processor.m_vertices)
		{
			vertices.push_back(vertex.pos.x);
			vertices.push_back(vertex.pos.y);
			vertices.push_back(vertex.pos.z);
			vertices.push_back(vertex.uv0.x);
			vertices.push_back(vertex.uv0.y);
			vertices.push_back(vertex.normal.x);
			vertices.push_back(vertex.normal.y);
			vertices.push_back(vertex.normal.z);
			vertices.push_back(vertex.tangent.x);
			vertices.push_back(vertex.tangent.y);
			vertices.push_back(vertex.tangent.z);
			vertices.push_back(vertex.tangent.w);
		}

		// update index bias.
		uint32_t lastMeshVertexIndex = lastMeshVerticesPos / getStandardMeshAttributesVertexCount();
		std::for_each(std::execution::par_unseq, processor.m_indices.begin(), processor.m_indices.end(), [lastMeshVertexIndex](auto&& index)
		{
			index = index + lastMeshVertexIndex;
		});

		m_cacheVerticesData.insert(m_cacheVerticesData.end(),vertices.begin(),vertices.end());
		m_cacheIndicesData.insert(m_cacheIndicesData.end(),processor.m_indices.begin(),processor.m_indices.end());

		if(prepareFallback)
		{
			newMesh->fallBackMesh = m_cacheMeshMap[engineMeshes::box.path].get();
			CHECK(newMesh->fallBackMesh != nullptr);
		}
		m_cacheMeshMap[info.path] = std::move(newMesh);

		return m_cacheMeshMap[info.path].get();
	}

	Mesh* MeshManager::getAssetMeshByUUID(const std::string& uuid)
	{
		if (m_cacheMeshMap.contains(uuid))
		{
			return m_cacheMeshMap[uuid].get();
		}

		std::string orignalPath = m_uuidMap.at(uuid);
		MeshInfo& meshInfo = m_assetMeshMap.at(orignalPath);
		std::string binPath,metaPath;
		if(getAssetPath(orignalPath,metaPath,binPath))
		{
			AssetBin assetBin {};
			if(!loadBinFile(binPath.c_str(),assetBin))
			{
				return nullptr;
			}

			std::vector<VertexIndexType> indicesData = {};
			std::vector<float> verticesData = {};
			
			unpackMeshBinFile(&meshInfo,assetBin.binBlob.data(),assetBin.binBlob.size(),verticesData,indicesData);
			auto newMesh = std::make_unique<Mesh>();
			m_unReadyMeshes.push_back(newMesh.get());

			newMesh->bReady = false;
			newMesh->fallBackMesh = m_cacheMeshMap[engineMeshes::box.path].get();
			CHECK(newMesh->fallBackMesh != nullptr);

			newMesh->layout = meshInfo.attributeLayout;
			newMesh->subMeshes.resize(meshInfo.subMeshCount);

			uint32_t lastMeshVerticesPos = (uint32_t)m_cacheVerticesData.size();
			uint32_t lastMeshIndexPos = (uint32_t)m_cacheIndicesData.size();

			for (uint32_t i = 0; i < meshInfo.subMeshCount; i++)
			{
				const auto& subMeshInfo = meshInfo.subMeshInfos[i];
				auto& subMesh = newMesh->subMeshes[i];

				subMesh.indexCount = subMeshInfo.indexCount;
				subMesh.indexStartPosition = subMeshInfo.indexStartPosition + lastMeshIndexPos;
				subMesh.renderBounds.extents = glm::vec3(
					subMeshInfo.bounds.extents[0],
					subMeshInfo.bounds.extents[1],
					subMeshInfo.bounds.extents[2]
				);

				subMesh.renderBounds.origin = glm::vec3(
					subMeshInfo.bounds.origin[0],
					subMeshInfo.bounds.origin[1],
					subMeshInfo.bounds.origin[2]
				);

				subMesh.renderBounds.radius = subMeshInfo.bounds.radius;
				
				if(m_assetSystem->getMaterialManager()->existMaterial(subMeshInfo.materialPath))
				{
					subMesh.material = subMeshInfo.materialPath;
				}
				else
				{
					subMesh.material = UNVALID_UUID;
				}
			}

			newMesh->vertexStartPosition = lastMeshVerticesPos;
			newMesh->vertexCount = (uint32_t)verticesData.size();
			newMesh->indexStartPosition = lastMeshIndexPos;
			newMesh->indexCount = (uint32_t)indicesData.size();

			uint32_t lastMeshVertexIndex = lastMeshVerticesPos / getStandardMeshAttributesVertexCount();
			std::for_each(std::execution::par_unseq,indicesData.begin(),indicesData.end(), [lastMeshVertexIndex](auto&& index)
			{
				index = index + lastMeshVertexIndex;
			});

			m_cacheVerticesData.insert(m_cacheVerticesData.end(),verticesData.begin(),verticesData.end());
			m_cacheIndicesData.insert(m_cacheIndicesData.end(),indicesData.begin(),indicesData.end());

			m_cacheMeshMap[uuid] = std::move(newMesh);
			return m_cacheMeshMap[uuid].get();
		}
		else
		{
			return nullptr;
		}
	}

	Mesh* MeshManager::getAssetMeshByMetaName(const std::string& metaName)
	{
		return getAssetMeshByUUID(m_assetMeshMap.at(metaName).UUID);
	}

	void MeshManager::init(AssetSystem* in)
	{
		m_assetSystem = in;

		CHECK(m_indexBuffer  == nullptr);
		CHECK(m_vertexBuffer == nullptr);

		VkDeviceSize baseVertexBufferSize = static_cast<VkDeviceSize>(cVarBaseVRamForVertexBuffer.get()) * 1024 * 1024;
		VkDeviceSize baseIndexBufferSize  = static_cast<VkDeviceSize>(cVarBaseVRamForIndexBuffer.get())  * 1024 * 1024;

		m_cacheVerticesData.reserve(1024 * 1024);
		m_cacheIndicesData.reserve(1024 * 1024);

		m_indexBuffer = VulkanIndexBuffer::create(
			RHI::get()->getVulkanDevice(),
			baseIndexBufferSize,
			RHI::get()->getGraphicsCommandPool(),
			true,
			true
		);

		m_vertexBuffer = VulkanVertexBuffer::create(
			RHI::get()->getVulkanDevice(),
			RHI::get()->getGraphicsCommandPool(),
			baseVertexBufferSize,
			true,
			true
		);

		// prepare fallback.
		getEngineMesh(engineMeshes::box, false);
		flushAppendBuffer();
	}

	void MeshManager::release()
	{
		// release engine meshes.
		m_cacheMeshMap.clear();

		if(m_indexBuffer)
		{
			delete m_indexBuffer;
			m_indexBuffer = nullptr;
		}

		if(m_vertexBuffer)
		{
			delete m_vertexBuffer;
			m_vertexBuffer = nullptr;
		}
	}

	void MeshManager::bindVertexBuffer(VkCommandBuffer cmd)
	{
		m_vertexBuffer->bind(cmd);
	}

	void MeshManager::bindIndexBuffer(VkCommandBuffer cmd)
	{
		m_indexBuffer->bind(cmd);
	}
}