#include "UISystem.h"
#include "Icon.h"

#include "../RHI/RHI.h"

namespace flower
{
	UISystem::UISystem(ModuleManager* in)
		: IRuntimeModule(in,"UISystem")
	{
		m_iconLibrary = std::make_unique<IconLibrary>();
	}

	bool UISystem::init()
	{
		m_iconLibrary->init();
		ImGuiCommon::init();

		return true;
	}

	void UISystem::tick(const TickData& tickData)
	{
		if(tickData.bIsMinimized)
		{
			return;
		}

		ImGuiCommon::newFrame();

		for(auto& callBackPair : m_uiFunctions)
		{
			callBackPair.second(tickData, m_frameIndex);
		}

		m_frameIndex ++;
		if(m_frameIndex >= getBackBufferCount())
		{
			m_frameIndex = 0;
		}
	}

	void UISystem::release()
	{
		ImGuiCommon::release();
		m_iconLibrary->release();
	}

	void UISystem::addImguiFunction(std::string name,std::function<RegisterImguiFunc>&& func)
	{
		CHECK(m_uiFunctions.find(name) == m_uiFunctions.end());

		m_uiFunctions[name] = func;
	}

	void UISystem::removeImguiFunction(std::string name)
	{
		CHECK(m_uiFunctions.find(name) != m_uiFunctions.end());

		m_uiFunctions.erase(name);
	}
}