#include "Bindless.h"
#include "RHI.h"

namespace flower 
{
	constexpr auto MAX_GPU_LOAD_TEXTURE_COUNT = 50000;
	constexpr auto MAX_GPU_LOAD_SAMPLER_COUNT = 500;

	uint32_t BindlessBase::getCountAndAndOne()
	{
		uint32_t index = 0;
		m_bindlessElementCountLock.lock();
		{
			index = m_bindlessElementCount;
			m_bindlessElementCount++;

			const auto maxCount = std::max(MAX_GPU_LOAD_TEXTURE_COUNT, MAX_GPU_LOAD_SAMPLER_COUNT);
			CHECK(m_bindlessElementCount < maxCount && "too much texture load in gpu!");
		}
		m_bindlessElementCountLock.unlock();
		return index;
	}

	VkDescriptorSetLayout BindlessBase::getSetLayout()
	{
		return m_bindlessDescriptorHeap.setLayout;
	}

	void BindlessBase::release()
	{
		vkDestroyDescriptorSetLayout(
			RHI::get()->getDevice(), 
			m_bindlessDescriptorHeap.setLayout,
			nullptr
		);

		vkDestroyDescriptorPool(
			RHI::get()->getDevice(),
			m_bindlessDescriptorHeap.descriptorPool,
			nullptr
		);
	}

	VkDescriptorSet BindlessBase::getSet()
	{
		return m_bindlessDescriptorHeap.descriptorSetUpdateAfterBind;
	}

	void BindlessTexture::init()
	{
		// create bindless binding here.
		VkDescriptorSetLayoutBinding binding{};
		binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		binding.binding = 0;

		// set max descriptor count to a big number.
		CHECK(MAX_GPU_LOAD_TEXTURE_COUNT < 
			RHI::get()->getPhysicalDeviceDescriptorIndexingProperties().maxDescriptorSetUpdateAfterBindSampledImages)
		binding.descriptorCount = MAX_GPU_LOAD_TEXTURE_COUNT;

		// one binding.
		VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
		setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setLayoutCreateInfo.bindingCount = 1;
		setLayoutCreateInfo.pBindings = &binding;
		setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

		const VkDescriptorBindingFlagsEXT flags =
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags{};
		bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		bindingFlags.bindingCount = 1;
		bindingFlags.pBindingFlags = &flags;
		setLayoutCreateInfo.pNext = &bindingFlags;

		vkCheck(vkCreateDescriptorSetLayout(
			RHI::get()->getDevice(),
			&setLayoutCreateInfo,
			nullptr,
			&m_bindlessDescriptorHeap.setLayout));

		VkDescriptorPoolSize  poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSize.descriptorCount = MAX_GPU_LOAD_TEXTURE_COUNT;

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;
		poolCreateInfo.maxSets = 1;
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;

		vkCheck(vkCreateDescriptorPool(*RHI::get()->getVulkanDevice(), &poolCreateInfo, nullptr, &m_bindlessDescriptorHeap.descriptorPool));

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = m_bindlessDescriptorHeap.descriptorPool;
		allocateInfo.pSetLayouts = &m_bindlessDescriptorHeap.setLayout;
		allocateInfo.descriptorSetCount = 1;

		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableInfo{};
		variableInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		variableInfo.descriptorSetCount = 1;
		allocateInfo.pNext = &variableInfo;

		const uint32_t NumDescriptorsTextures = MAX_GPU_LOAD_TEXTURE_COUNT;
		variableInfo.pDescriptorCounts = &NumDescriptorsTextures;
		vkCheck(vkAllocateDescriptorSets(*RHI::get()->getVulkanDevice(), &allocateInfo, &m_bindlessDescriptorHeap.descriptorSetUpdateAfterBind));
	}

	uint32_t BindlessTexture::updateTextureToBindlessDescriptorSet(Texture2DImage* in)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = VK_NULL_HANDLE;

		imageInfo.imageView   = in->getImageView();
		imageInfo.imageLayout = in->getCurentLayout();

		VkWriteDescriptorSet  write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = getSet();
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.dstBinding = 0;
		write.pImageInfo = &imageInfo;
		write.descriptorCount = 1;
		write.dstArrayElement = getCountAndAndOne();

		vkUpdateDescriptorSets(RHI::get()->getDevice(), 1, &write, 0, nullptr);

		return write.dstArrayElement;
	}

	void BindlessSampler::init()
	{
		// create bindless binding here.
		VkDescriptorSetLayoutBinding binding{};
		binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		binding.binding = 0;

		// set max descriptor sampler count to a big number.
		CHECK(MAX_GPU_LOAD_SAMPLER_COUNT <
			RHI::get()->getPhysicalDeviceDescriptorIndexingProperties().maxDescriptorSetUpdateAfterBindSamplers)
		binding.descriptorCount = MAX_GPU_LOAD_SAMPLER_COUNT;

		// one binding.
		VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo{};
		setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		setLayoutCreateInfo.bindingCount = 1;
		setLayoutCreateInfo.pBindings = &binding;
		setLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

		const VkDescriptorBindingFlagsEXT flags =
			VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags{};
		bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		bindingFlags.bindingCount = 1;
		bindingFlags.pBindingFlags = &flags;
		setLayoutCreateInfo.pNext = &bindingFlags;

		vkCheck(vkCreateDescriptorSetLayout(
			RHI::get()->getDevice(),
			&setLayoutCreateInfo,
			nullptr,
			&m_bindlessDescriptorHeap.setLayout));

		VkDescriptorPoolSize  poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSize.descriptorCount = MAX_GPU_LOAD_SAMPLER_COUNT;

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;
		poolCreateInfo.maxSets = 1;
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;

		vkCheck(vkCreateDescriptorPool(*RHI::get()->getVulkanDevice(), &poolCreateInfo, nullptr, &m_bindlessDescriptorHeap.descriptorPool));

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = m_bindlessDescriptorHeap.descriptorPool;
		allocateInfo.pSetLayouts = &m_bindlessDescriptorHeap.setLayout;
		allocateInfo.descriptorSetCount = 1;

		VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableInfo{};
		variableInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
		variableInfo.descriptorSetCount = 1;
		allocateInfo.pNext = &variableInfo;

		const uint32_t NumDescriptorsTextures = MAX_GPU_LOAD_SAMPLER_COUNT;
		variableInfo.pDescriptorCounts = &NumDescriptorsTextures;
		vkCheck(vkAllocateDescriptorSets(*RHI::get()->getVulkanDevice(), &allocateInfo, &m_bindlessDescriptorHeap.descriptorSetUpdateAfterBind));
	}

	uint32_t BindlessSampler::updateSamplerToBindlessDescriptorSet(VkSampler in)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = in;
		imageInfo.imageView = VK_NULL_HANDLE;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkWriteDescriptorSet  write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = getSet();
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		write.dstBinding = 0;
		write.pImageInfo = &imageInfo;
		write.descriptorCount = 1;
		write.dstArrayElement = getCountAndAndOne();

		vkUpdateDescriptorSets(RHI::get()->getDevice(), 1, &write, 0, nullptr);

		return write.dstArrayElement;
	}

}