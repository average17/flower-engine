#include "../../WidgetCommon.h"

#include "ComponentCommon.h"

#include <Engine/Scene/SceneNode.h>
#include <Engine/Scene/Component/StaticMeshComponent.h>

#include <Engine/Asset/AssetMesh.h>
#include <Engine/Asset/AssetSystem.h>
#include <Engine/UI/UISystem.h>
#include <Engine/Engine.h>
#include <Engine/UI/Icon.h>

void drawStaticMeshSelect(std::shared_ptr<flower::SceneNode> n, std::shared_ptr<flower::StaticMeshComponent> comp)
{
	using namespace flower;

	if (ImGui::BeginMenu("Engine"))
	{
		float sz = ImGui::GetTextLineHeight();

		auto* meshManager = GEngine.getRuntimeModule<AssetSystem>()->getMeshManager();
		auto* iconLib = GEngine.getRuntimeModule<UISystem>()->getIconLibrary();

		
		auto meshItemShow = [&](const engineMeshes::EngineMeshInfo& info, uint32_t colorIndex)
		{
			const char* name = info.displayName.c_str();

			IconInfo* icon = iconLib->getIconInfo(info.snapShotPath);

			ImVec2 p = ImGui::GetCursorScreenPos();

			ImGui::Image((ImTextureID)icon->getId(),ImVec2(sz, sz),ImVec2(0,1),ImVec2(1,0));
			ImGui::SameLine();

			if(ImGui::MenuItem(name))
			{
				comp->setMeshId(info);
			}
		};

		meshItemShow(engineMeshes::box, 0);
		meshItemShow(engineMeshes::sphere, 1);
		meshItemShow(engineMeshes::cylinder, 2);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Project"))
	{
		float sz = ImGui::GetTextLineHeight();

		auto* meshManager = GEngine.getRuntimeModule<AssetSystem>()->getMeshManager();
		auto* iconLib = GEngine.getRuntimeModule<UISystem>()->getIconLibrary();

		const auto& projectMeshesMap = meshManager->getProjectMeshInfos();

		// i think it is no so good to loop whole map,
		// but imgui seems can't only loop visible items.
		for(const auto& pair : projectMeshesMap)
		{
			const char* name = pair.second.originalFile.c_str();

			IconInfo* icon = iconLib->getIconInfo(engineMeshes::box.snapShotPath);

			ImVec2 p = ImGui::GetCursorScreenPos();

			ImGui::Image((ImTextureID)icon->getId(),ImVec2(sz, sz),ImVec2(0,1),ImVec2(1,0));
			ImGui::SameLine();

			if(ImGui::MenuItem(name))
			{
				comp->setMeshId(pair.second.UUID);
			}
		}

		ImGui::EndMenu();
	}
}

void inspectStaticMesh(std::shared_ptr<flower::SceneNode> n)
{
	using namespace flower;

	if(std::shared_ptr<StaticMeshComponent> meshComponent = n->getComponent<StaticMeshComponent>())
	{
		auto activeScene = n->getSceneReference().lock();

		drawComponent<StaticMeshComponent>("StaticMeshComponent", n, *activeScene, [&](auto& component)
		{
			ImGui::Separator();
			{
				if (ImGui::Button("StaticMesh.."))
					ImGui::OpenPopup("StaticMeshSelectPopUp");
				if (ImGui::BeginPopup("StaticMeshSelectPopUp"))
				{
					drawStaticMeshSelect(n, component);
					ImGui::EndPopup();
				}
			}

			ImGui::SameLine();

			ImVec2 p = ImGui::GetCursorScreenPos();
			float sz = ImGui::GetTextLineHeight() * 5.0f;

			const bool bDrawEngineMeshSnapShot = component->isEngineMesh();
			if(bDrawEngineMeshSnapShot)
			{
				auto* iconLib = GEngine.getRuntimeModule<UISystem>()->getIconLibrary();
				IconInfo* icon = iconLib->getIconInfo(component->getEngineMeshInfo().snapShotPath);
				ImGui::Image((ImTextureID)icon->getId(),ImVec2(sz, sz),ImVec2(0,1),ImVec2(1,0));
			}
			else
			{
				ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), ImGui::GetColorU32((ImGuiCol)4));
				ImGui::Dummy(ImVec2(sz, sz));
			}

			ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(151, 145, 253, 255), 0.0f,0, 2.0f);

			ImGui::Separator();
		},true);
	}
}
