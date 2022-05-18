#include "Fence.h"
namespace flower
{
	VulkanFence::VulkanFence(VkDevice device,VulkanFencePool* pool,bool setSignaledOnCreated) : 
		m_fence(VK_NULL_HANDLE),
		m_state(setSignaledOnCreated ? State::Signaled : State::UnSignaled),
		m_pool(pool)
	{
		VkFenceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.flags = setSignaledOnCreated ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
		vkCreateFence(device, &createInfo, nullptr, &m_fence);
	}

	VulkanFence::~VulkanFence()
	{
		if(m_fence != VK_NULL_HANDLE)
		{
			LOG_WARN("Unrelease fence!");
		}
	}

	void VulkanFencePool::destroyFence(VulkanFence* fence)
	{
		vkDestroyFence(m_device, fence->m_fence, nullptr);
		fence->m_fence = VK_NULL_HANDLE;
		delete fence;
	}

	bool VulkanFencePool::checkFenceState(VulkanFence* fence)
	{
		VkResult result = vkGetFenceStatus(m_device, fence->m_fence);
		switch (result)
		{
		case VK_SUCCESS:
			fence->m_state = VulkanFence::State::Signaled;
			break;
		case VK_NOT_READY:
			break;
		default:
			break;
		}
		return false;
	}

	VulkanFencePool::VulkanFencePool()
	: m_device(VK_NULL_HANDLE)
	{
	}

	VulkanFencePool::~VulkanFencePool()
	{
		if(m_busyFences.size()!=0)
		{
			LOG_WARN("No all busy fences finish!");
		}
	}

	bool VulkanFencePool::isFenceSignaled(VulkanFence* fence)
	{
		if (fence->signaled()) 
		{
			return true;
		}
		return checkFenceState(fence);
	}

	void VulkanFencePool::init(VkDevice device)
	{
		m_device = device;
	}

	void VulkanFencePool::release()
	{
		vkDeviceWaitIdle(m_device);
		CHECK(m_busyFences.size() <= 0);

		for (int32_t i = 0; i < m_freeFences.size(); ++i) 
		{
			destroyFence(m_freeFences[i]);
		}
	}

	VulkanFence* VulkanFencePool::createFence(bool signaledOnCreate)
	{
		if (m_freeFences.size() > 0)
		{
			VulkanFence* fence = m_freeFences.back();
			m_freeFences.pop_back();
			m_busyFences.push_back(fence);
			if (signaledOnCreate) 
			{
				fence->m_state = VulkanFence::State::Signaled;
			}
			return fence;
		}

		VulkanFence* newFence = new VulkanFence(m_device, this, signaledOnCreate);
		m_busyFences.push_back(newFence);
		return newFence;
	}

	bool VulkanFencePool::waitForFence(VulkanFence* fence,uint64_t timeInNanoseconds)
	{
		VkResult result = vkWaitForFences(m_device, 1, &fence->m_fence, true, timeInNanoseconds);
		switch (result)
		{
		case VK_SUCCESS:
			fence->m_state = VulkanFence::State::Signaled;
			return true;
		case VK_TIMEOUT:
			return false;
		default:
			LOG_WARN("Unkow error {0}!", (int32_t)result);
			return false;
		}
	}

	void VulkanFencePool::resetFence(VulkanFence* fence)
	{
		if (fence->m_state != VulkanFence::State::UnSignaled)
		{
			vkResetFences(m_device, 1, &fence->m_fence);
			fence->m_state = VulkanFence::State::UnSignaled;
		}
	}

	void VulkanFencePool::releaseFence(VulkanFence*& fence)
	{
		resetFence(fence);
		for (int32_t i = 0; i < m_busyFences.size(); ++i) 
		{
			if (m_busyFences[i] == fence)
			{
				m_busyFences.erase(m_busyFences.begin() + i);
				break;
			}
		}
		m_freeFences.push_back(fence);
		fence = nullptr;
	}

	void VulkanFencePool::waitAndReleaseFence(VulkanFence*& fence,uint64_t timeInNanoseconds)
	{
		if (!fence->signaled()) 
		{
			waitForFence(fence, timeInNanoseconds);
		}
		releaseFence(fence);
	}
}