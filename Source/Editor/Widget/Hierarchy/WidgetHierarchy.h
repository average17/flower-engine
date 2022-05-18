#pragma once
#include "../WidgetInterface.h"

#include <ImGui/ImGuiInternal.h>
#include <ImGui/ImGui.h>
#include <Engine/Scene/SceneNode.h>

namespace flower
{
	class SceneManager;
}

class WidgetHierarhcy;

class EditorHierarchy
{
	friend WidgetHierarhcy;
private:
	static std::weak_ptr<flower::SceneNode> gSelectedNode;

public:
	static std::weak_ptr<flower::SceneNode> getSelectNode() { return gSelectedNode; }
};

class WidgetHierarhcy : public Widget
{
public:
	WidgetHierarhcy(flower::Engine* engine);
	virtual ~WidgetHierarhcy() noexcept;

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
	void drawScene();
	void drawTreeNode(std::shared_ptr<flower::SceneNode> node, flower::Scene* scene);

	void handleEvent();
	void popupMenu();

	void setSelectedNode(std::weak_ptr<flower::SceneNode> node);

	void actionNodeDelete(const std::shared_ptr<flower::SceneNode>& node);
	std::shared_ptr<flower::SceneNode> actionNodeCreateEmpty(const std::shared_ptr<flower::SceneNode>& node);

private:
	flower::SceneManager* m_sceneManager = nullptr;

	inline static std::string s_hierarchyDragDropId = "HierarchyDragDropNode";
	enum { UNVALID_ID = ~0 };

	// renameing input buffer.
	char m_inputBuffer[32];

	bool m_bExpandToSelection = false;
	bool m_bExpandedToSelection = false;
	bool m_bRenameing = false;

	std::weak_ptr<flower::SceneNode>      m_hoverNode;
	std::weak_ptr<flower::SceneNode>  m_leftClickNode;
	std::weak_ptr<flower::SceneNode> m_rightClickNode; // right click

	struct DragDropWrapper
	{
		size_t id;
		std::weak_ptr<flower::SceneNode> dragingNode;
	};
	DragDropWrapper m_dragingNode;

	ImRect m_selectedNodeRect;
};