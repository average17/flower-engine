#pragma once
#include "WidgetInterface.h"

class DockSpace : public Widget
{
public:
	DockSpace(flower::Engine* engine);
	~DockSpace() noexcept;

	// Dockspace always visible.
	virtual void onTick(const flower::TickData& tickData, size_t) override;
};