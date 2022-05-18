#include "WidgetInspector.h"
#include <ImGui/IconsFontAwesome5.h>
#include <ImGui/ImGui.h>
#include <Engine/Engine.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/SceneNode.h>
#include "../Hierarchy/WidgetHierarchy.h"
#include "Components/ComponentCommon.h"
#include <Engine/Scene/Component/DirectionalLightComponent.h>
#include <Engine/Scene/Component/AtmosphereSky.h>
#include <Engine/Scene/Component/Transform.h>
#include <Engine/Scene/Component/StaticMeshComponent.h>
#include <Engine/Scene/SceneGraph.h>

using namespace flower;

std::string INSPECTOR_TILE_ICON = ICON_FA_INFO;

WidgetInspector::WidgetInspector(flower::Engine* engine)
	: Widget(engine, " " + INSPECTOR_TILE_ICON + "  Inspector")
{

}

WidgetInspector::~WidgetInspector() noexcept
{

}

void WidgetInspector::onInit()
{
	m_sceneManager = GEngine.getRuntimeModule<SceneManager>();
}

void WidgetInspector::onTick(const flower::TickData& tickData,size_t)
{

}

void WidgetInspector::onRelease()
{

}

void WidgetInspector::onVisibleTick(const flower::TickData& tickData,size_t)
{
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(getTile().c_str(), &m_bShow))
	{
		ImGui::End();
		return;
	}

	if(const auto selectNode = EditorHierarchy::getSelectNode().lock())
	{
		if(!selectNode->isRoot())
		{
			inspectTransform(selectNode);

			inspectAtmospherSky(selectNode);
			inspectDirectionalLight(selectNode);

			inspectStaticMesh(selectNode);

			drawAddCommand(selectNode);
		}
	}


	ImGui::End();
}

void WidgetInspector::drawAddCommand(std::shared_ptr<flower::SceneNode> node)
{
	auto activeScene = node->getSceneReference().lock();

	ImGui::Spacing();
	const char* addCompName = "Add Component";
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize(addCompName).x * 0.5f);
	if (ImGui::Button(addCompName))
	{
		ImGui::OpenPopup("##ComponentContextMenu_Add");
	}

	if (ImGui::BeginPopup("##ComponentContextMenu_Add"))
	{
		// Atmosphere Sky
		if (ImGui::MenuItem("AtmosphereSky") && !node->hasComponent<AtmosphereSky>())
		{
			activeScene->addComponent(std::make_shared<AtmosphereSky>(),node);
			activeScene->setDirty(true);
			LOG_INFO("Node {0} has add atmosphere sky component.", node->getName());
		}

		// Directional Light
		if (ImGui::MenuItem("DirectionalLight") && !node->hasComponent<DirectionalLight>())
		{
			activeScene->addComponent(std::make_shared<DirectionalLight>(),node);
			activeScene->setDirty(true);
			LOG_INFO("Node {0} has add directional light component.", node->getName());
		}

		// StaticMesh
		if (ImGui::MenuItem("StaticMesh") && !node->hasComponent<StaticMeshComponent>())
		{
			activeScene->addComponent(std::make_shared<StaticMeshComponent>(),node);
			activeScene->setDirty(true);
			LOG_INFO("Node {0} has add static mesh component.", node->getName());
		}

		ImGui::EndPopup();
	}
}

