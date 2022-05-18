#pragma once

#include "Common.h"
#include "ContentViewItem.h"
#include "ContentTreeItem.h"

class ContentView
{
	friend ContentViewItem;
	friend ContentTreeItem;

private:
	// current working directory, only set on main thread.
	std::string m_workingDirectory;
	std::string m_projectDirectory;

	EntryInfoCompare m_entryInfoCompare{ };

	// widget reference.
	WidgetAsset* m_widget;

	// detail for content views.
	ViewItemDetail m_itemDetail {};

	// is hovering some content view?
	bool m_bHoveringWindow = false;

	// current working path dirty?
	bool m_bNeedUpdated = true;

	// current working path's content items.
	std::vector<ContentViewItem> m_contentItems;

	// current project reflect tree.
	std::shared_ptr<ContentTreeItem> m_contentTreeRoot;
	
private:
	void viewContentDraw();

	void buildContentTree();
	void buildContentTreeRecursive(std::shared_ptr<ContentTreeItem> parent);

	void drawTreeRecursive(std::shared_ptr<ContentTreeItem>);
public:
	void init(std::string projectDir, WidgetAsset* widget);

	void drawTree();
	void drawContent();
	void setDirty();

	
};