#include "Icon.h"

#include <Engine/RHI/RHI.h>
#include <ImGui/ImGuiVulkan.h>

using namespace flower;

ImTextureID ImageInfo::getId(uint32_t i, bool bRebuild)
{ 
	if(m_cacheId.size() != getBackBufferCount())
	{
		m_cacheId.resize(getBackBufferCount());
		for(size_t t = 0; t < m_cacheId.size(); t++)
		{
			m_cacheId[t] = nullptr;
		}
	}

	bool bBuild = false;
	for(size_t t = 0; t < m_cacheId.size(); t++)
	{
		if(m_cacheId[t] == nullptr)
		{
			bBuild = true;
		}
	} 

	if(bRebuild)
	{
		// free last frame cache id and use new id.
		for(size_t t = 0; t < m_cacheId.size(); t++)
		{
			if(m_cacheId[t] != nullptr)
			{
				ImGui_ImlVulkan_FreeSet(m_cacheId[t]);
				m_cacheId[t] = nullptr;
				bBuild = true;
			}
		}
	}

	if(bBuild)
	{
		VkImageViewCreateInfo viewInfo{};
		{
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_icon->getImage();

			// all icon view type is 2d.
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_icon->getFormat();

			VkImageSubresourceRange subres{};

			// all icon aspectMask is color.
			subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			// all icon no layer and no mipmap.
			subres.baseArrayLayer = 0;
			subres.levelCount = 1;
			subres.layerCount = 1;
			viewInfo.subresourceRange = subres;
		}

		VkImageView iconView = m_icon->requestImageView(viewInfo);

		CHECK((iconView != VK_NULL_HANDLE) && "No valid image view can use for image.");

		for(size_t t = 0; t < m_cacheId.size(); t++)
		{
			m_cacheId[t] = ImGui_ImplVulkan_AddTexture(
				m_sampler, 
				iconView, 
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}
	}
	
	return m_cacheId[i];
}
