#pragma once
#include "../../Core/Core.h"
#include "../../RHI/RHI.h"

// Scene static uniform buffer.
// ring style buffer.
namespace flower
{
	struct GPUFrameData
	{
		alignas(16) glm::vec4 appTime;
			// .x is app runtime
			// .y is game runtime
			// .z is sin(.y)
			// .w is cos(.y)

		alignas(16) glm::vec4 directionalLightDir;
		alignas(16) glm::vec4 directionalLightColor;

		alignas(16) glm::uvec4 frameIndex;
			// .x is frame count
			// .y is frame count % 8
			// .z is frame count % 16
			// .w is frame count % 32

		alignas(16) glm::vec4  jitterData = glm::vec4(0.0f);
			// .xy is current frame jitter data
			// .zw is prev frame jitter data

		alignas(16) glm::vec4 switch0 = glm::vec4(0.0f);
			// .x is TAA open.
	};

	struct GPUViewData
	{
		alignas(16) glm::vec4 camWorldPos;
		alignas(16) glm::vec4 camInfo; 
			// .x fovy
			// .y aspectRatio
		    // .z nearZ
			// .w farZ

		alignas(16) glm::mat4 camView;
		alignas(16) glm::mat4 camProj;
		alignas(16) glm::mat4 camViewProj = glm::mat4(1.0f);

		alignas(16) glm::mat4 camViewInverse;
		alignas(16) glm::mat4 camProjInverse;
		alignas(16) glm::mat4 camViewProjInverse;

		// prev frame camera view project matrix.
		// no jitter.
		alignas(16) glm::mat4 camViewProjLast;

		alignas(16) glm::mat4 camViewProjJitter;
		alignas(16) glm::mat4 camProjJitter;
		alignas(16) glm::mat4 camProjJitterInverse;
		alignas(16) glm::mat4 camInvertViewProjectionJitter;

		alignas(16) glm::vec4 frustumPlanes[6];
	};

	struct GPUDrawCallData
	{
		uint32_t indexCount;
		uint32_t instanceCount;
		uint32_t firstIndex;
		uint32_t vertexOffset;

		uint32_t firstInstance;
		uint32_t objectId;
	};

	struct GPUDepthRange
	{
		uint32_t minDepth;
		uint32_t maxDepth;
	};

	struct GPUCascadeInfo
	{
		alignas(16) glm::mat4 cascadeViewProjMatrix;
		alignas(16) glm::vec4 frustumPlanes[6];
	};

	struct GPUOutIndirectDrawCount
	{
		uint32_t outDrawCount;
	};

	// same with glsl.
	constexpr int32_t MAX_CACSADE_COUNT = 8;
	constexpr int32_t MAX_SSBO_OBJECTS  = 20000;
	constexpr int32_t MAX_CASCADE_SSBO_OBJECTS = MAX_SSBO_OBJECTS * MAX_CACSADE_COUNT;

	struct GPUObjectData
	{
		alignas(16) glm::mat4 modelMatrix;
		alignas(16) glm::mat4 prevModelMatrix;
		alignas(16) glm::vec4 sphereBounds;
		alignas(16) glm::vec4 extents;

		uint32_t indexCount;
		uint32_t firstIndex;
		uint32_t vertexOffset;
		uint32_t firstInstance;
	};

	struct GPUMaterialData
	{
		uint32_t shadingModel;
		float cutoff;

		uint32_t  baseColorId;
		uint32_t  baseColorSampler;

		uint32_t normalTexId;   
		uint32_t normalSampler;

		uint32_t specularTexId; 
		uint32_t specularSampler;

		uint32_t emissiveTexId; 
		uint32_t emissiveSampler;
	};

	template<typename SSBOType, size_t Count>
	struct SingleSSBO
	{
		VulkanBuffer* ssbo;
		static constexpr VkDeviceSize size = sizeof(SSBOType) * Count;

		VulkanDescriptorSetReference set = {};
		VulkanDescriptorLayoutReference setLayout = {};
		
		void init(uint32_t bindPos, bool bHost = false)
		{
			ssbo = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				bHost ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_GPU_ONLY,
				bHost ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				this->size,
				nullptr
			);

			VkDescriptorBufferInfo bufInfo = {};
			bufInfo.buffer = *this->ssbo;
			bufInfo.offset = 0;
			bufInfo.range = this->size;
			RHI::get()->vkDescriptorFactoryBegin()
				.bindBuffer(bindPos, &bufInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
				.build(set, setLayout);
		}

		void release()
		{
			delete ssbo;
		}
	};

	// for us, we use tri-buffer ssbo.
	// so we can update them before sync frame buffer.
	template<typename SSBOType, size_t BACK_BUFFER_COUNT, size_t SSBOCount = MAX_SSBO_OBJECTS>
	class SceneUploadSSBO
	{
	private:
		static_assert(BACK_BUFFER_COUNT > 0 && "Backbuffer count should bigger than zero.");

		size_t m_ringIndex = 0;

		std::array<VulkanBuffer*, BACK_BUFFER_COUNT> m_bufferRing;

		// default set. binding default set to zero.
		std::array<VulkanDescriptorSetReference, BACK_BUFFER_COUNT> m_setRing;
		VulkanDescriptorLayoutReference m_layout;

	public:
		constexpr size_t getSSBOSize() const
		{
			return sizeof(SSBOType) * SSBOCount;
		}

		void init()
		{
			auto bufferSize = getSSBOSize();

			for(uint32_t index = 0; index < BACK_BUFFER_COUNT; index++)
			{
				m_bufferRing[index] = VulkanBuffer::create(
					RHI::get()->getVulkanDevice(),
					RHI::get()->getGraphicsCommandPool(),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					bufferSize,
					nullptr
				);

				VkDescriptorBufferInfo frameDataBufInfo = {};
				frameDataBufInfo.buffer = m_bufferRing[index]->getVkBuffer();
				frameDataBufInfo.offset = 0;
				frameDataBufInfo.range = bufferSize;

				RHI::get()->vkDescriptorFactoryBegin()
					.bindBuffer(0, &frameDataBufInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
					.build(m_setRing[index], m_layout);
			}
		}

		void release()
		{
			for(auto* buffer : m_bufferRing)
			{
				delete buffer;
			}
		}

		void onFrameBegin()
		{
			m_ringIndex ++;
			if(m_ringIndex >= BACK_BUFFER_COUNT)
			{
				m_ringIndex = 0;
			}
		}

		VulkanBuffer* getBuffer()
		{
			return m_bufferRing[m_ringIndex];
		}

		VulkanDescriptorSetReference getSet()
		{
			return m_setRing[m_ringIndex];
		}

		VulkanDescriptorLayoutReference getLayout()
		{
			return m_layout;
		}
	};

	// use for uniform buffer.
	template<typename T, size_t BACK_BUFFER_COUNT>
	class RenderRingData
	{
		static_assert(BACK_BUFFER_COUNT > 0 && "Backbuffer count should bigger than zero.");
		
	private:
		size_t m_ringIndex = 0;

		std::array<VulkanBuffer*, BACK_BUFFER_COUNT> m_bufferRing;

		// default set. binding default set to zero.
		std::array<VulkanDescriptorSetReference, BACK_BUFFER_COUNT> m_setRing;
		VulkanDescriptorLayoutReference m_layout;

	public:
		void init()
		{
			constexpr size_t bufferSize = sizeof(T);

			for(uint32_t index = 0; index < BACK_BUFFER_COUNT; index++)
			{
				m_bufferRing[index] = VulkanBuffer::create(
					RHI::get()->getVulkanDevice(),
					RHI::get()->getGraphicsCommandPool(),
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VMA_MEMORY_USAGE_CPU_TO_GPU,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					bufferSize,
					nullptr
				);

				VkDescriptorBufferInfo frameDataBufInfo = {};
				frameDataBufInfo.buffer = m_bufferRing[index]->getVkBuffer();
				frameDataBufInfo.offset = 0;
				frameDataBufInfo.range = bufferSize;

				// default set to binding zero.
				// we use rhi static descriptor set factory.
				RHI::get()->vkDescriptorFactoryBegin()
					.bindBuffer(0, &frameDataBufInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
					.build(m_setRing[index], m_layout);
			}
		}

		void release()
		{
			// release buffer.
			for(auto* buffer : m_bufferRing)
			{
				delete buffer;
			}
		}

		void onFrameBegin()
		{
			m_ringIndex ++;
			if(m_ringIndex >= BACK_BUFFER_COUNT)
			{
				m_ringIndex = 0;
			}
		}

		VulkanBuffer* getBuffer()
		{
			return m_bufferRing[m_ringIndex];
		}

		VulkanDescriptorSetReference getSet()
		{
			return m_setRing[m_ringIndex];
		}

		VulkanDescriptorLayoutReference getLayout()
		{
			return m_layout;
		}

		void updateData(const T& in)
		{
			T data = in;
			constexpr size_t bufferSize = sizeof(T);

			getBuffer()->map(bufferSize);
			getBuffer()->copyTo(&data, bufferSize);
			getBuffer()->unmap();
		}
	};

	template<
		typename IndirectType, 
		size_t SSBOBufferCount,
		typename CountType, 
		size_t CountBufferCount>
	struct DrawIndirectBuffer
	{
		VulkanBuffer* drawIndirectSSBO;
		VulkanDescriptorSetReference descriptorSets = {};
		VulkanDescriptorLayoutReference descriptorSetLayout = {};
		static constexpr VkDeviceSize size = sizeof(IndirectType) * SSBOBufferCount;

		VulkanBuffer* countBuffer;
		VulkanDescriptorSetReference countDescriptorSets = {};
		VulkanDescriptorLayoutReference countDescriptorSetLayout = {};
		static constexpr VkDeviceSize countSize = sizeof(CountType) * CountBufferCount;

		void init(uint32_t bindingPos = 0, uint32_t countBindingPos = 0)
		{
			this->drawIndirectSSBO = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				this->size,
				nullptr
			);
			VkDescriptorBufferInfo bufInfo = {};
			bufInfo.buffer = *this->drawIndirectSSBO;
			bufInfo.offset = 0;
			bufInfo.range = this->size;
			RHI::get()->vkDescriptorFactoryBegin()
				.bindBuffer(bindingPos, &bufInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
				.build(descriptorSets, descriptorSetLayout);
			
			this->countBuffer = VulkanBuffer::create(
				RHI::get()->getVulkanDevice(),
				RHI::get()->getGraphicsCommandPool(),
				VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VMA_MEMORY_USAGE_GPU_ONLY,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				countSize,
				nullptr
			);
			VkDescriptorBufferInfo countBufInfo = {};
			countBufInfo.buffer = *this->countBuffer;
			countBufInfo.offset = 0;
			countBufInfo.range = this->countSize;
			RHI::get()->vkDescriptorFactoryBegin()
				.bindBuffer(countBindingPos, &countBufInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
				.build(countDescriptorSets, countDescriptorSetLayout);
		}

		void release()
		{
			delete drawIndirectSSBO;
			delete countBuffer;
		}
	};
}