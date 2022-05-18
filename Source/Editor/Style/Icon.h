#pragma once
#include <string>
#include <Engine/Core/UtilsPath.h>
#include <vulkan/vulkan.h>
#include <ImGui/ImGui.h>

namespace IconPath
{
	inline static const std::string editorIconFolder     = "./Image/Editor/";
	inline static const std::string editorIconFolderFull = flower::UtilsPath::getUtilsPath()->getEngineWorkingPath() + editorIconFolder;

	// declare icon name here.
	inline static const std::string folderIcon     = editorIconFolderFull + "./folder.png";
	inline static const std::string fileIcon       = editorIconFolderFull + "./file.png";
};

namespace flower
{
	class VulkanImage;
}

struct ImageInfo
{
private:
	flower::VulkanImage* m_icon = nullptr;

	VkSampler m_sampler;
	std::vector<ImTextureID> m_cacheId { };

public:
	void setIcon(flower::VulkanImage* i)
	{
		m_icon = i;
	}

	void setSampler(VkSampler s)
	{
		m_sampler = s;
	}

	ImTextureID getId(uint32_t i, bool bRebuild = false);
};