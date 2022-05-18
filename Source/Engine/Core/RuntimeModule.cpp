#include "RuntimeModule.h"
#include "Core.h"

namespace flower {

	ModuleManager::~ModuleManager()
	{
		// destruct from end to start.
		for (size_t i = m_runtimeModules.size() - 1; i > 0; i--)
		{
			m_runtimeModules[i].reset();
		}

		m_runtimeModules.clear();
	}

	bool ModuleManager::init()
	{
		bool result = true;

		for (const auto& runtimeModule : m_runtimeModules)
		{
			if (!runtimeModule->init())
			{
				LOG_ERROR("Runtime module {0} failed to init.", typeid(*runtimeModule).name());
				result = false;
			}
		}
		return result;
	}

	void ModuleManager::tick(const TickData& tickData)
	{
		for (const auto& runtimeModule : m_runtimeModules)
		{
			runtimeModule->tick(tickData);
		}
	}

	void ModuleManager::release()
	{
		for (size_t i = m_runtimeModules.size(); i > 0; i--)
		{
			m_runtimeModules[i - 1]->release();
		}
	}

}