#include "ContentTreeItem.h"

ContentTreeItem::ContentTreeItem(
	const EntryInfo& info,
	WidgetAsset* widget,
	ContentView* view, 
	std::shared_ptr<ContentTreeItem> parent)
: m_widget(widget), m_view(view), m_entryInfo(info), m_parent(parent)
{

}
