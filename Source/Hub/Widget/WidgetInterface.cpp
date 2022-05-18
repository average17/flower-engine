#include "WidgetInterface.h"
#include <Engine/Core/Core.h>
#include <Engine/Engine.h>
#include <Engine/Renderer/Renderer.h>
#include <Engine/UI/UISystem.h>

using namespace flower;

Widget::Widget(flower::Engine* engine, std::string tile) :
	m_engine(engine), m_title(tile), m_bShow(true), m_bLastShow(true)
{
	m_uiSystem = m_engine->getRuntimeModule<UISystem>();

	CHECK(m_uiSystem && "Please register ui system for editor application.");
}

void Widget::init()
{
	// register tick function on uisystem.
	m_uiSystem->addImguiFunction(m_title,[this](const TickData& tickData, size_t i)
	{
		this->tick(tickData, i);
	});

	onInit();
}

void Widget::tick(const TickData& tickData, size_t frameIndex)
{
	// sync visible state.
	if(m_bLastShow != m_bShow)
	{
		m_bLastShow = m_bShow;
		if(m_bShow)
		{
			// last time is hide, current show.
			onShow();
		}
		else
		{
			// last time show, current hide.
			onHide();
		}
	}

	onTick(tickData, frameIndex);

	if(m_bShow)
	{
		onVisibleTick(tickData, frameIndex);
	}
}

void Widget::release()
{
	// unregister tick function.
	m_uiSystem->removeImguiFunction(m_title);

	onRelease();
}