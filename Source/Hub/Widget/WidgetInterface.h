#pragma once
#include <string>
#include <Engine/Core/TickData.h>

namespace flower
{
	class Engine;
	class UISystem;
}

class Widget
{
private:
	std::string m_title;
	bool m_bLastShow;

protected:
	bool m_bShow;
	flower::Engine*   m_engine;
	flower::UISystem* m_uiSystem;

public:
	std::string getTile() const
	{
		return m_title;
	}

	flower::UISystem* getUISystem()
	{
		return m_uiSystem;
	}

	flower::Engine* getEngine()
	{
		return m_engine;
	}

public:
	Widget() = delete;
	Widget(flower::Engine*, std::string tile);
	virtual ~Widget() {  }

	void init();
	void release();

private:
	void tick(const flower::TickData& tickData, size_t);

protected:
	// event init.
	virtual void onInit() { }

	// event always tick.
	virtual void onTick(const flower::TickData& tickData, size_t) {  }

	// event release.
	virtual void onRelease() {  }

	// event on widget visible state change. sync on tick function first.
	virtual void onHide() {  }
	virtual void onShow() {  }

	// event when widget visible tick.
	virtual void onVisibleTick(const flower::TickData& tickData, size_t) {  }
}; 