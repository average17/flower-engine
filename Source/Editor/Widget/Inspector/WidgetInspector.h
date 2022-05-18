#pragma once
#include "../WidgetInterface.h"
#include <memory>

namespace flower
{
	class SceneManager;
	class SceneNode;
}

class WidgetInspector : public Widget
{
public:
	WidgetInspector(flower::Engine* engine);
	virtual ~WidgetInspector() noexcept;

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
	flower::SceneManager* m_sceneManager = nullptr;

	void drawAddCommand(std::shared_ptr<flower::SceneNode> node);
};