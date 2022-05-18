#pragma once
#include <atomic>
#include <memory>

namespace flower
{
	// single producer, single consumer.
	// lock free queue
	template<typename T>
	class LockFreeSPSCQueue
	{
	private:
		struct Node
		{
			std::shared_ptr<T> data = nullptr;
			Node* next = nullptr;
		};

		std::atomic<Node*> m_head;
		std::atomic<Node*> m_tail;

		Node* popHead()
		{
			Node* const oldHead = m_head.load();

			// when pop head, maybe push tail also happening,
			// but we don't care about this,
			// if we miss this time, next time we still can pop the 
			// newest old tail.
			if(oldHead == m_tail.load())
			{
				// if empty then return nullptr.
				return nullptr;
			}
			m_head.store(oldHead->next);
			return oldHead;
		}
	
	public:
		LockFreeSPSCQueue():
			m_head(new Node()),m_tail(m_head.load())
		{

		}

		LockFreeSPSCQueue(const LockFreeSPSCQueue& rhs) = delete;
		LockFreeSPSCQueue& operator=(const LockFreeSPSCQueue& rhs) = delete;

		~LockFreeSPSCQueue()
		{
			while(Node* const oldHead = m_head.load())
			{
				m_head.store(oldHead->next);
				delete oldHead;
			}
		}

		std::shared_ptr<T> pop()
		{
			Node* oldHead = popHead();
			if(!oldHead)
			{
				// if empty then return null shared ptr.
				return nullptr;
			}

			// else copy the data of node to the return value.
			std::shared_ptr<T> const res(oldHead->data);
			delete oldHead;

			return res;
		}

		void push(T newVal)
		{
			std::shared_ptr<T> newData(std::make_shared<T>(newVal));

			Node* p = new Node();

			// same as pop head, don't care about race here.
			Node* const oldTail = m_tail.load();
			oldTail->data.swap(newData);

			oldTail->next = p;
			m_tail.store(p);
		}
	};
}