#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>

#include "../Core/NonCopyable.h"
#include "../Core/Delegates.h"

namespace flower
{
	class AsyncThread : NonCopyable
	{
	public:
		AsyncThread() = delete;

		AsyncThread(size_t period, std::string);
		virtual ~AsyncThread() { }

	private:
		size_t m_period;
		std::string m_name;
		std::thread m_thread;

	public:
		DelegatesThreadSafe<AsyncThread> callbacks;

	private:
		void loop();
	};

	extern AsyncThread GSlowAsyncThread; // tick per 1  s.
	extern AsyncThread GFastAsyncThread; // tick per 10 ms.

	// otherwise, if you want to no delay thread, just use job system.
}