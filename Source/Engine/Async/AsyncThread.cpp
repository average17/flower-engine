#include "AsyncThread.h"
#include "../Core/WindowData.h"

namespace flower
{
	AsyncThread GSlowAsyncThread(1000, "SlowAsyncThread");
	AsyncThread GFastAsyncThread(10,   "FastAsyncThread");

	AsyncThread::AsyncThread(size_t period, std::string name)
	: m_period(period), m_name(name)
	{
		m_thread = std::thread(&AsyncThread::loop, this);
		m_thread.detach();

		LOG_TRACE("Start async thread: {}.", name);
	}

	void AsyncThread::loop()
	{
		while (true)
		{
			// this is singleton so don't care about it's life.
			if(!GLFWWindowData::get()->shouldRun())
			{
				LOG_TRACE("Async thread {} exiting.", m_name);
				return;
			}

			// do job.
			callbacks.broadcast();

			if(m_period > 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(m_period));
			}
		}
	}
}