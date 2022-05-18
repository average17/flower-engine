#pragma once
#include <ImGui/ImGui.h>
#include <ImGui/ImGuiInternal.h>
#include <memory>
#include <Engine/Scene/SceneNode.h>
#include <Engine/Scene/Component/Transform.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Scene/SceneGraph.h>
#include <ImGui/IconsFontAwesome5.h>

template<typename T, typename UIFunction>
static void drawComponent(
	const std::string& name, 
	std::shared_ptr<flower::SceneNode> node, 
	flower::Scene& scene, 
	UIFunction uiFunction,
	bool removable)
{
	using namespace flower;

	const ImGuiTreeNodeFlags treeNodeFlags = 
		ImGuiTreeNodeFlags_DefaultOpen | 
		ImGuiTreeNodeFlags_Framed | 
		ImGuiTreeNodeFlags_SpanAvailWidth | 
		ImGuiTreeNodeFlags_AllowItemOverlap | 
		ImGuiTreeNodeFlags_FramePadding;

	if (node->hasComponent<T>())
	{
		std::shared_ptr<T> component = node->getComponent<T>();

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

		ImGui::Separator();
		bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
		ImGui::PopStyleVar();

		ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
		if (ImGui::Button(ICON_FA_PLUS, ImVec2{ lineHeight, lineHeight }))
		{
			ImGui::OpenPopup("ComponentSettings");
		}

		bool removeComponent = false;
		if (ImGui::BeginPopup("ComponentSettings"))
		{
			if (ImGui::MenuItem("Remove Component"))
				removeComponent = removable;

			ImGui::EndPopup();
		}

		if (open)
		{
			uiFunction(component);
			ImGui::TreePop();
		}

		if(removeComponent)
		{
			scene.removeComponent<T>(node);
		}
	}
}

namespace flower
{
	class SceneNode;
}

extern void inspectAtmospherSky(std::shared_ptr<flower::SceneNode> n);
extern void inspectDirectionalLight(std::shared_ptr<flower::SceneNode> n);
extern void inspectStaticMesh(std::shared_ptr<flower::SceneNode> n);
extern void inspectTransform(std::shared_ptr<flower::SceneNode> n);
