#pragma once
#include <atomic>
#include "../Core/Core.h"

namespace flower
{
	// owner thread call push(), pop() on bottom.
	// other thread call steal() on top.
	template<typename T>
	class WorkStealingQueue
	{
	private:
		T** m_items;
		size_t m_capacity;

		std::atomic<int64_t> m_top {0};
		std::atomic<int64_t> m_bottom {0};

	public:
		static size_t bufferSize(size_t capacity)
		{
			return capacity * sizeof(T*);
		}

		bool init(size_t capacity,void* buffer,size_t inBufferSize)
		{
			// must be power of 2.
			if((capacity & (capacity-1)) != 0) return false; 

			// in buffer's size must bigger than queue request.
			size_t minBufferSize = bufferSize(capacity);
			if (inBufferSize < minBufferSize) return false; 

			uint8_t* bufferNext = (uint8_t*)buffer;

			// assign buffer address to m_items.
			m_items = (T**)bufferNext;
			bufferNext += capacity * sizeof(T*);

			// buffer offset should keep same with request size.
			CHECK(bufferNext - (uint8_t*)buffer == (intptr_t)minBufferSize);

			// clear init value.
			for(int i = 0; i < capacity; i++) 
			{
				m_items[i] = nullptr;
			}
			m_capacity = capacity;

			return true;
		}

		// push new item from owner thread.
		void push(T* newitem)
		{
			// we only push on owner thread so just relaxed order is enough.
			int64_t id = m_bottom.load(std::memory_order_relaxed);
			m_items[id & (m_capacity-1)] = newitem;

			// need memory_order_release, because queue's item maybe stealing by other thread.
			// m_bottom maybe read on other thread on stealing.
			m_bottom.store(id +1, std::memory_order_release);
		}

		// pop call on owner thread.
		T* pop()
		{
			int64_t bottom = m_bottom.fetch_sub(1, std::memory_order_seq_cst) - 1;
			CHECK(bottom >= -1);

			// case special: maybe last item stealing by other thread...
			{
				// stealing....
			}

			int64_t top = m_top.load(std::memory_order_seq_cst);

			if(top < bottom)
			{
				return m_items[bottom & (m_capacity - 1)];
			}

			T* item = nullptr;
			if(top == bottom)
			{
				item = m_items[bottom & (m_capacity - 1)];

				// last item, maybe stealed by other other thread.
				if(m_top.compare_exchange_strong(top, top + 1, 
					std::memory_order_seq_cst,
					std::memory_order_relaxed))
				{
					// if success, no steal by other, but we pop new one.
					// m_top now equal to top + 1. 
					// so update top to keep same with m_top.
					top ++;
				}
				else
				{
					// fail, stole by other.
					// top now is newest m_top.
					item = nullptr;
				}
			}
			else
			{
				// on case special.
				// queue already empty, stole by other.
				CHECK(top - bottom == 1);
			}

			// queue empty, and top is newest m_top.

			// update bottom.
			m_bottom.store(top, std::memory_order_relaxed);
			return item;
		}

		T* steal()
		{
			while (true) 
			{
				int64_t top = m_top.load(std::memory_order_seq_cst);
				int64_t bottom = m_bottom.load(std::memory_order_seq_cst);

				if (top >= bottom) 
				{
					// queue is empty
					return nullptr;
				}

				// The queue isn't empty
				T* item = m_items[top & (m_capacity - 1)];

				if (m_top.compare_exchange_strong(top, top + 1,
					std::memory_order_seq_cst,
					std::memory_order_relaxed)) 
				{
					// stole success. 
					return item;
				}

				// try again.
			}
		}
	};
}