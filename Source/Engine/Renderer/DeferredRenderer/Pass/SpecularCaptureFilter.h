#pragma once
#include "PassCommon.h"
#include "../SceneTextures.h"

namespace flower
{
	class TextureManager;
	class SpecularCaptureFilterPass : public GraphicsPass
	{
	public:
		SpecularCaptureFilterPass(DefferedRenderer* renderer)
			: GraphicsPass(renderer, "SpecularCaptureFilterPass")
		{

		}

		void filterImage(RenderTextureCube* inCube, CombineTextureBindless* srcImage);

	private:
		virtual void initInner() override {  }
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) override {}
		virtual void releaseInner() override;
		TextureManager* m_textureManager = nullptr;

	private:
		VkPipeline m_pipelines = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayouts = VK_NULL_HANDLE;
		VulkanDescriptorLayoutReference m_descriptorSetLayouts = {};
	};
}