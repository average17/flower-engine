#include "ComponentCommon.h"
#include "../../WidgetCommon.h"
#include <Engine/Scene/SceneNode.h>
#include <Engine/Scene/Component/Transform.h>

void inspectTransform(std::shared_ptr<flower::SceneNode> n)
{
	using namespace flower;

	if(std::shared_ptr<Transform> transform = n->getComponent<Transform>())
	{
		auto activeScene = n->getSceneReference().lock();

		glm::vec3 translation = transform->getTranslation();
		glm::quat rotation = transform->getRotation();
		glm::vec3 scale = transform->getScale();
		glm::vec3 rotate_degree = glm::degrees(glm::eulerAngles(rotation));

		drawComponent<Transform>("Transform", n, *activeScene, [&](auto& component)
		{
			ImGui::Separator();
			WidgetCommon::drawVector3("P", translation, glm::vec3(0.0f));
			ImGui::Separator();
			WidgetCommon::drawVector3("R", rotate_degree, glm::vec3(0.0f));
			ImGui::Separator();
			WidgetCommon::drawVector3("S", scale, glm::vec3(1.0f));
			ImGui::Separator();
		},false); // Transform is un-removable.

		if(translation != transform->getTranslation())
		{
			transform->setTranslation(translation);
			activeScene->setDirty(true);
		}

		if(scale != transform->getScale())
		{
			transform->setScale(scale);
			activeScene->setDirty(true);
		}

		rotation = glm::quat(glm::radians(rotate_degree));
		if(rotation != transform->getRotation())
		{
			transform->setRotation(rotation);
			activeScene->setDirty(true);
		}
	}
}
