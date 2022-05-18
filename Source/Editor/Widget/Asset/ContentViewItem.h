#pragma once

#include "Common.h"
#include <Engine/Core/Core.h>

class ContentViewItem
{
private:
	WidgetAsset* m_widget;
	ContentView* m_view;
	EntryInfo m_entryInfo;
	bool m_bHoveringItem;
	flower::IconInfo* prepareIcon(const EntryInfo& info, flower::IconLibrary* lib);
	glm::uvec4 prepareRectColor(const EntryInfo& info);

	struct ThumbnailInfo
	{
		std::chrono::duration<double,std::milli> timeSinceLastClick;
		std::chrono::time_point<std::chrono::high_resolution_clock> lastClickTime;
	};
	ThumbnailInfo m_thumbnailInfo;

public:
	ContentViewItem(const EntryInfo& info,WidgetAsset* widget,ContentView* view);

	void draw(const ViewItemDetail& detail);
	bool shouldDraw() const;
};