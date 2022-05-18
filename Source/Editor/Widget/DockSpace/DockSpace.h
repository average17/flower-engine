#pragma once
#include "../WidgetInterface.h"
#include "Menu.h"

class DockSpace : public Widget
{
public:
	DockSpace(flower::Engine* engine);
	~DockSpace() noexcept;

	// Dockspace always visible.
	virtual void onTick(const flower::TickData& tickData, size_t) override;

	void init();
private:
	Menu m_menu { };
};