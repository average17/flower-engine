#pragma once
#include "../WidgetInterface.h"
#include "../../Style/Icon.h"

#include <ImGui/ImGuiInternal.h>
#include <ImGui/ImGui.h>

namespace flower
{
	class DefferedRenderer;
}

class WidgetViewport : public Widget
{
public:
	WidgetViewport(flower::Engine* engine);
	virtual ~WidgetViewport() noexcept;

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
	ImageInfo m_cacheImageInfo;
	flower::DefferedRenderer* m_renderer;

	bool m_bNeedRebuild = false;
};