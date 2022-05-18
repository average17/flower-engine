#include "WidgetHierarchy.h"

#include <Engine/Core/UtilsPath.h>
#include <Engine/Async/AsyncThread.h>
#include <Engine/Core/Core.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/SceneGraph.h>
#include <Engine/Engine.h>

#include <ImGui/IconsFontAwesome5.h>

using namespace flower;

std::weak_ptr<SceneNode> EditorHierarchy::gSelectedNode = {};

std::string HIERARHCY_TILE_ICON = ICON_FA_BUILDING;
std::string ROOT_NODE_ICON      = ICON_FA_HOME;
std::string EMPTY_NODE_ICON     = ICON_FA_FAN;

WidgetHierarhcy::WidgetHierarhcy(flower::Engine* engine)
	: Widget(engine, " " + HIERARHCY_TILE_ICON + "  Hierarchy")
{
	m_sceneManager = m_engine->getRuntimeModule<SceneManager>();
}

WidgetHierarhcy::~WidgetHierarhcy() noexcept
{

}

void WidgetHierarhcy::onInit()
{

}

void WidgetHierarhcy::onTick(const flower::TickData& tickData,size_t)
{

}

void WidgetHierarhcy::onRelease()
{

}

void WidgetHierarhcy::onVisibleTick(const flower::TickData& tickData,size_t)
{
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(getTile().c_str(), &m_bShow))
	{
		ImGui::End();
		return;
	}

	drawScene();

	if (ImGui::IsMouseReleased(0) && m_leftClickNode.lock() && m_leftClickNode.lock()->getId() != UNVALID_ID)
	{
		if (m_hoverNode.lock() && m_hoverNode.lock()->getId() == m_leftClickNode.lock()->getId())
		{
			setSelectedNode(m_leftClickNode);
		}
		m_leftClickNode.reset();
	}
	m_bExpandToSelection = true;

	ImGui::End();
}

void WidgetHierarhcy::drawScene()
{
	m_hoverNode.reset();

	std::vector<Scene*> readyScenes = m_sceneManager->getReadyScenes();

	ImGui::Separator();
	for (auto* scene : readyScenes)
	{
		CHECK(scene);
		auto rootNode = scene->getRootNode();
		drawTreeNode(rootNode, scene);
		ImGui::Separator();
	}

	if (m_bExpandToSelection && !m_bExpandedToSelection)
	{
		ImGui::ScrollToBringRectIntoView(ImGui::GetCurrentWindow(), m_selectedNodeRect);
		m_bExpandToSelection = false;
	}

	handleEvent();
	popupMenu();
}

void WidgetHierarhcy::drawTreeNode(std::shared_ptr<SceneNode> node, flower::Scene* scene)
{
	CHECK((node->getSceneReference().lock().get() == scene) && "Please don't operate node cross scenes.");

	const bool bRootNode = node->getDepth() == 0;
	const bool bTreeNode = node->getChildren().size() > 0;

	m_bExpandedToSelection = false;
	bool bSelectedNode = false;

	ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanFullWidth;
	nodeFlags |= bTreeNode ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf;

	if (const auto selectedNode = EditorHierarchy::gSelectedNode.lock())
	{
		nodeFlags |= selectedNode->getId() == node->getId() ? ImGuiTreeNodeFlags_Selected : nodeFlags;

		// expanded whole scene tree to select item.
		if (m_bExpandToSelection)
		{
			if (selectedNode->getParent() && selectedNode->getParent()->getId() == node->getId())
			{
				ImGui::SetNextItemOpen(true);
				m_bExpandedToSelection = true;
			}
		}
	}

	bool bEditingName = false;
	bool bNodeOpen;
	if (m_bRenameing && EditorHierarchy::gSelectedNode.lock() && EditorHierarchy::gSelectedNode.lock()->getId() == node->getId())
	{
		bNodeOpen = true;
		bEditingName = true;
		strcpy(m_inputBuffer, node->getName().c_str());
		if (ImGui::InputText(" ", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
		{
			node->setName(m_inputBuffer);
			m_bRenameing = false;
		}

		if (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			m_bRenameing = false;
		}
	}
	else
	{
		std::string iconName;
		if(bRootNode)
		{
			iconName = ROOT_NODE_ICON;
		}
		else
		{
			iconName = EMPTY_NODE_ICON;
		}
		std::string name = iconName+"  "+node->getName();

		bNodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(node->getId())), nodeFlags,name.c_str());
	}

	if ((nodeFlags & ImGuiTreeNodeFlags_Selected) && m_bExpandToSelection)
	{
		m_selectedNodeRect = ImGui::GetContext()->LastItemData.Rect;
	}

	// update hover node.
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
	{
		m_hoverNode = node;
	}

	// start drag drop.
	if (ImGui::BeginDragDropSource())
	{
		m_dragingNode.dragingNode = node;
		m_dragingNode.id = node->getId();

		ImGui::SetDragDropPayload(s_hierarchyDragDropId.c_str(), &m_dragingNode.id, sizeof(size_t));

		ImGui::Text(node->getName().c_str());
		
		ImGui::EndDragDropSource();
	}

	// accept drag drop.
	if (const auto hoverActiveNode = m_hoverNode.lock())
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_hierarchyDragDropId.c_str()))
			{
				if (const auto payloadNode = m_dragingNode.dragingNode.lock())
				{
					if (m_dragingNode.id != UNVALID_ID)
					{
						// todo: replace with command.
						if (scene->setParent(hoverActiveNode, payloadNode))
						{
							// reset draging node.
							m_dragingNode.dragingNode.reset();
							m_dragingNode.id = UNVALID_ID;
						}
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	if (bNodeOpen)
	{
		if (bTreeNode)
		{
			for (const auto& child : node->getChildren())
			{
				drawTreeNode(child, scene);
			}
		}

		if (!bEditingName)
		{
			ImGui::TreePop();
		}
	}
}

void WidgetHierarhcy::handleEvent()
{
	const auto bWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	if (!bWindowHovered)
	{
		return;
	}

	const auto bLeftClick   = ImGui::IsMouseClicked(0);
	const auto bRightClick  = ImGui::IsMouseClicked(1);
	const auto bDoubleClick = ImGui::IsMouseDoubleClicked(0);

	// update left click node.
	if (bLeftClick && m_hoverNode.lock())
	{
		m_leftClickNode = m_hoverNode;
	}

	// update right click node.
	if (bRightClick && m_hoverNode.lock())
	{
		m_rightClickNode = m_hoverNode;
	}

	// update selected node.
	if (bLeftClick || bDoubleClick || bRightClick)
	{
		if (m_hoverNode.lock())
		{
			setSelectedNode(m_hoverNode);
		}
	}

	if (bDoubleClick && EditorHierarchy::gSelectedNode.lock() && EditorHierarchy::gSelectedNode.lock()->getId() != UNVALID_ID)
	{
		m_bRenameing = true;
	}

	if (bRightClick)
	{
		ImGui::OpenPopup("##HierarchyContextMenu");
	}

	// update selected node to empty if no hover node.
	if ((bRightClick || bLeftClick) && !m_hoverNode.lock())
	{
		setSelectedNode({});
	}
}

void WidgetHierarhcy::popupMenu()
{
	auto ContextMenu = [&]()
	{
		if (auto selectedNode = EditorHierarchy::gSelectedNode.lock())
		{
			if (!ImGui::BeginPopup("##HierarchyContextMenu"))
				return;

			if (ImGui::MenuItem("Rename"))
			{
				m_bRenameing = true;
			}

			if (ImGui::MenuItem("Delete"))
			{
				actionNodeDelete(selectedNode);
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Create Empty Node"))
			{
				actionNodeCreateEmpty(selectedNode);
			}

			ImGui::EndPopup();
		}
	};

	ContextMenu();
}

void WidgetHierarhcy::setSelectedNode(std::weak_ptr<flower::SceneNode> node)
{
	m_bExpandToSelection = true;

	EditorHierarchy::gSelectedNode = node;
}

void WidgetHierarhcy::actionNodeDelete(const std::shared_ptr<flower::SceneNode>& node)
{
	if (auto scene = node->getSceneReference().lock())
	{
		// skip root node.
		if (node->getDepth() != 0)
		{
			scene->deleteNode(node);
		}
	}
}

std::shared_ptr<flower::SceneNode> WidgetHierarhcy::actionNodeCreateEmpty(const std::shared_ptr<flower::SceneNode>& node)
{
	auto scene = node->getSceneReference().lock();
	auto selectedNode = EditorHierarchy::gSelectedNode.lock();

	CHECK(scene && selectedNode && "please ensure scene valid and selected node valid when create new node.");

	std::string name = "SceneNode:" + std::to_string(scene->getCurrentGUID() + 1);
	auto newNode = scene->createNode(name.c_str());

	scene->setParent(selectedNode, newNode);
	scene->setDirty();

	return newNode;
}
