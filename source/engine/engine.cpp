#include "Engine.h"
#include "Core/Core.h"

namespace flower {

	Engine GEngine = {};

	bool Engine::beforeInit()
	{
		CHECK(m_moduleManager == nullptr);

		m_moduleManager = std::make_unique<ModuleManager>();
		m_moduleManager->m_engine = this;

		return true;
	}

	bool Engine::init()
	{
		CHECK(m_moduleManager);

		m_moduleManager->init();

		m_bInit = true;
		return true;
	}

	Engine::~Engine()
	{
		m_moduleManager.reset();
	}

	bool Engine::tick(const TickData& tickData)
	{
		m_moduleManager->tick(tickData);
		return true;
	}

	void Engine::release()
	{
		m_moduleManager->release();
	}
}