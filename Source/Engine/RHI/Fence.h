#pragma once
#include "Error.h"

namespace flower
{
	class VulkanFencePool;
	class VulkanFence
	{
		friend VulkanFencePool;
	public:
		enum class State
		{
			Signaled,
			UnSignaled,
		};
	private:
		VkFence m_fence;
		State m_state;
		VulkanFencePool* m_pool;

	public:
		VulkanFencePool* getPool() { return m_pool; }
		inline bool signaled() const { return m_state == State::Signaled; }
		inline VkFence getFence() const { return m_fence; }
		VulkanFence(VkDevice device,VulkanFencePool* pool,bool setSignaledOnCreated = false);
		~VulkanFence();

	};

	class VulkanFencePool
	{
	private:
		VkDevice m_device;
		std::vector<VulkanFence*> m_freeFences;
		std::vector<VulkanFence*> m_busyFences;

	private:
		void destroyFence(VulkanFence* fence);
		bool checkFenceState(VulkanFence* fence);

	public:
		VulkanFencePool();
		~VulkanFencePool();
		bool isFenceSignaled(VulkanFence* fence);
		void init(VkDevice device);
		void release();
		VulkanFence* createFence(bool signaledOnCreate);
		bool waitForFence(VulkanFence* fence, uint64_t timeInNanoseconds = 1000000);
		void resetFence(VulkanFence* fence);
		void releaseFence(VulkanFence*& fence);

		void waitAndReleaseFence(VulkanFence*& fence, uint64_t timeInNanoseconds = 1000000);
	};

}