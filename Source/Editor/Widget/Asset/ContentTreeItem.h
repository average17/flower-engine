#pragma once

#include "Common.h"

class ContentTreeItem : std::enable_shared_from_this<ContentTreeItem>
{
	friend ContentView;

private:
	WidgetAsset* m_widget;
	ContentView* m_view;

	EntryInfo m_entryInfo;

	std::weak_ptr<ContentTreeItem> m_parent;
	std::vector<std::shared_ptr<ContentTreeItem>> m_children;

public:
	ContentTreeItem(const EntryInfo& info, WidgetAsset* widget, ContentView* view, std::shared_ptr<ContentTreeItem> parent);
	
	void addChild(std::shared_ptr<ContentTreeItem> child)
	{
		m_children.push_back(child);
	}
};
