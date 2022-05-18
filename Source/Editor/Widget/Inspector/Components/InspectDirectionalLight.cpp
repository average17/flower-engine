#include "ComponentCommon.h"
#include "../../WidgetCommon.h"
#include <Engine/Scene/SceneNode.h>
#include <Engine/Scene/Component/DirectionalLightComponent.h>

void inspectDirectionalLight(std::shared_ptr<flower::SceneNode> n)
{
	using namespace flower;

	if(std::shared_ptr<DirectionalLight> light = n->getComponent<DirectionalLight>())
	{
		auto activeScene = n->getSceneReference().lock();

		drawComponent<DirectionalLight>("DirectionalLight", n, *activeScene, [&](auto& component)
		{
			ImGui::Separator();
			{
				glm::vec3 oldColor = component->getColor();
				ImVec4 color = ImVec4(oldColor.x, oldColor.y, oldColor.z, 1.0f);
				ImGui::ColorPicker4("Color", (float*)&color, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoBorder);
				glm::vec3 newColor = glm::vec3(color.x,color.y,color.z);

				if(newColor != oldColor)
				{
					component->setColor(newColor);
				}

				// forward.
			}
			ImGui::Separator();
		},true);
	}
}
