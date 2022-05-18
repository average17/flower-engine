#pragma once
#include "../WidgetInterface.h"
#include "Console.h"

#include <memory>

class WidgetConsole : public Widget
{
public:
	WidgetConsole(flower::Engine* engine);
	virtual ~WidgetConsole() noexcept;

protected:
	// event init.
	virtual void onInit() override;

	// event always tick.
	virtual void onTick(const flower::TickData& tickData, size_t) override;

	// event release.
	virtual void onRelease() override;

	// event when widget visible tick.
	virtual void onVisibleTick(const flower::TickData& tickData, size_t) override;

private:
	std::unique_ptr<Console> m_console;
};