#pragma once
#include <mutex>

#include "ContentView.h"
#include "../WidgetInterface.h"

namespace flower
{
	class AssetSystem;
}

class WidgetAsset : public Widget
{
public:
	void setProjectDir(std::string projectDir);

private:
	// only set on main thread.
	std::string m_projectDir; 

	const char* m_onProjectDirtyCallbackName = "AssetWidgetOnProjectDirty";

	ContentView m_contentView { };

	flower::AssetSystem* m_assetSystem;

public:
	WidgetAsset(flower::Engine* engine);
	virtual ~WidgetAsset() noexcept;

	flower::AssetSystem* getAssetSystem() const { return m_assetSystem; }

protected:

	// event init.
	virtual void onInit() override;

	// event always tick.
	virtual void onTick(const flower::TickData& tickData, size_t) override;

	// event release.
	virtual void onRelease() override;

	// event when widget visible tick.
	virtual void onVisibleTick(const flower::TickData& tickData, size_t) override;
};