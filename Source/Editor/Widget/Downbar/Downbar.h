#pragma once
#include "../WidgetInterface.h"

class WidgetDownbar : public Widget
{
public:
	WidgetDownbar(flower::Engine* engine);
	~WidgetDownbar() noexcept;

	// Dockspace always visible.
	virtual void onTick(const flower::TickData& tickData, size_t) override;
};