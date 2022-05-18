#pragma once
#include "WidgetInterface.h"

#include <ImGui/ImGui.h>

class WidgetDemo : public Widget
{
public:
	WidgetDemo(flower::Engine* engine)
	:Widget(engine, "Demo")
	{

	}

	virtual ~WidgetDemo() noexcept
	{

	}

protected:
	// event init.
	virtual void onInit() override
	{

	}

	// event always tick.
	virtual void onTick(const flower::TickData& tickData,size_t) override
	{

	}

	// event release.
	virtual void onRelease() override
	{

	}

	// event when widget visible tick.
	virtual void onVisibleTick(const flower::TickData& tickData,size_t) override
	{
		ImGui::ShowDemoWindow(&m_bShow);
	}
};