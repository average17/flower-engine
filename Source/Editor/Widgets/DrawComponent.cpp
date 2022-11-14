#include "Pch.h"
#include "Detail.h"
#include "DrawComponent.h"

using namespace Flower;
using namespace Flower::UI;

const std::string GIconLandscape = std::string("  ") + ICON_FA_MOUNTAIN_SUN + std::string("  Landscape");
const std::string GIconDirectionalLight = std::string("  ") + ICON_FA_SUN + std::string("  DirectionalLight");
const std::string GIconSpotLight = std::string("  ") + ICON_FA_SUN + std::string("  SpotLight");
const std::string GIconStaticMesh = std::string("   ") + ICON_FA_BUILDING + std::string("   StaticMesh");
const std::string GIconPMX = std::string("   ") + ICON_FA_M + ICON_FA_I + ICON_FA_K + ICON_FA_U + std::string("   PMX");

void drawLandscape(std::shared_ptr<SceneNode> node)
{
	auto t = 1.0f;
	ImGui::DragFloat("holder", &t, 0.1f);
	ImGui::DragFloat("holder", &t, 0.1f);
	ImGui::DragFloat("holder", &t, 0.1f);
}

std::unordered_map<std::string, ComponentDrawer> GDrawComponentMap = 
{
	{ GIconPMX, { typeid(PMXComponent).name(), &ComponentDrawer::drawPMX }},
	{ GIconLandscape, { typeid(LandscapeComponent).name(), &drawLandscape }},
	{ GIconDirectionalLight, { typeid(DirectionalLightComponent).name(), &ComponentDrawer::drawDirectionalLight }},
	{ GIconSpotLight, { typeid(SpotLightComponent).name(), &ComponentDrawer::drawSpotLight }},
	{ GIconStaticMesh, { typeid(StaticMeshComponent).name(), &ComponentDrawer::drawStaticMesh }},
};