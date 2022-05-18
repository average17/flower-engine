#pragma once
#include <map>
#include <mutex>
#include <functional>

namespace flower
{
	template<typename Friend, typename...Args>
	class DelegatesSingleThread
	{
		friend Friend;
	public:
		using DelegatesFunction = std::function<void(Args...)>;

	private:
		std::map<std::string, DelegatesFunction> m_callbacks;

	public:
		[[nodiscard]] bool registerFunction(const std::string& name, DelegatesFunction&& callback)
		{
			if(m_callbacks.contains(name))
			{
				return false;
			}
			else
			{
				m_callbacks[name] = callback;
				return true;
			}
		}

		[[nodiscard]] bool unregisterFunction(const std::string& name)
		{
			if(m_callbacks.contains(name))
			{
				m_callbacks.erase(name);
				return true;
			}
			else
			{
				return false;
			}
		}

	private:
		void broadcast(Args&&... args)
		{
			if(m_callbacks.size()>0)
			{
				for(auto& callback : m_callbacks)
				{
					if(callback.second)
					{
						callback.second(std::forward<Args>(args)...);
					}
				}
			}
		}
	};

	template<typename Friend, typename...Args>
	class DelegatesThreadSafe
	{
		friend Friend;
	public:
		using DelegatesFunction = std::function<void(Args...)>;

	private:
		std::mutex m_mutex;
		std::map<std::string, DelegatesFunction> m_callbacks;

	public:
		[[nodiscard]] bool registerFunction(const std::string& name, DelegatesFunction&& callback)
		{
			std::lock_guard guard(m_mutex);

			if(m_callbacks.contains(name))
			{
				return false;
			}
			else
			{
				m_callbacks[name] = callback;
			}
			return true;
		}

		[[nodiscard]] bool unregisterFunction(const std::string& name)
		{
			std::lock_guard guard(m_mutex);

			if(m_callbacks.contains(name))
			{
				m_callbacks.erase(name);
			}
			else
			{
				return false;
			}
			return true;
		}

	private:
		void broadcast(Args&&... args)
		{
			std::lock_guard guard(m_mutex);

			if(m_callbacks.size()>0)
			{
				for(auto& callback : m_callbacks)
				{
					if(callback.second)
					{
						callback.second(std::forward<Args>(args)...);
					}
				}
			}
		}
	};
}