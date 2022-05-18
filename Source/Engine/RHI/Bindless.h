#pragma once
#include <vulkan/vulkan.h>
#include <mutex>

namespace flower
{
	class BindlessBase
	{
	protected:
		struct BindlessTextureDescriptorHeap
		{
			VkDescriptorSetLayout setLayout{};
			VkDescriptorPool descriptorPool{};
			VkDescriptorSet descriptorSetUpdateAfterBind{};
		};
		BindlessTextureDescriptorHeap m_bindlessDescriptorHeap;

		uint32_t m_bindlessElementCount = 0;
		std::mutex m_bindlessElementCountLock;

	public:
		virtual ~BindlessBase() {};

		uint32_t getCountAndAndOne();
		VkDescriptorSet getSet();
		VkDescriptorSetLayout getSetLayout();

		virtual void init() = 0;
		virtual void release();
	};

	class Texture2DImage;
	class BindlessTexture : public BindlessBase
	{
	public:
		virtual ~BindlessTexture() {};

		virtual void init() override;

		// only texture2d image use bindless set.
		// return bindless index.
		uint32_t updateTextureToBindlessDescriptorSet(Texture2DImage* in);
	};

	class BindlessSampler : public BindlessBase
	{
	public:
		virtual ~BindlessSampler() {};
		virtual void init() override;

		// register sampler to bindless descriptor set.
		// return bindless index.
		uint32_t updateSamplerToBindlessDescriptorSet(VkSampler in);
	};
}