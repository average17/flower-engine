#include "ComponentCommon.h"
#include "../../WidgetCommon.h"
#include <Engine/Scene/SceneNode.h>
#include <Engine/Scene/Component/AtmosphereSky.h>

void inspectAtmospherSky(std::shared_ptr<flower::SceneNode> n)
{
	using namespace flower;

	if(std::shared_ptr<AtmosphereSky> light = n->getComponent<AtmosphereSky>())
	{
		auto activeScene = n->getSceneReference().lock();

		drawComponent<AtmosphereSky>("AtmosphereSky", n, *activeScene, [&](auto& component)
		{
			ImGui::Separator();
			{
				ImGui::SliderFloat("exposure",&component->params.atmosphereExposure, 0.1f, 100.0f, "%.1f");
			
				ImGui::ColorEdit3("GroundAlbedo", &component->params.groundAlbedo.x);
				ImGui::ColorEdit3("OzoneAbsorptionBase", &component->params.ozoneAbsorptionBase.x);
				ImGui::SliderFloat("ozone Altitude", &component->params.ozoneAltitude, 0.0f, 50.0f, "%.3f");
				ImGui::SliderFloat("ozone Thickness", &component->params.ozoneThickness, 0.0f, 50.0f, "%.3f");
				ImGui::SliderFloat("GroundRadiusMM",&component->params.groundRadiusMM, 0.0f, 10.0f, "%.3f");
				ImGui::SliderFloat("AtmosphereRadiusMM",&component->params.atmosphereRadiusMM, 0.0f, 10.0f, "%.3f");
				ImGui::SliderFloat("OffsetHeight",&component->params.offsetHeight, 0.0f, 0.1f, "%.6f");
			}
			ImGui::Separator();
			{
				ImGui::Text("Rayleigh");
				ImGui::ColorEdit3("Rayleigh ScatteringBase", &component->params.rayleighScatteringBase.x);
				ImGui::SliderFloat("Rayleigh AbsorptionBase",&component->params.rayleighAbsorptionBase, 0.0f, 1.0f, "%.5f");
				ImGui::SliderFloat("Rayleigh Altitude", &component->params.rayleighAltitude, 0.0f, 20.0f, "%.5f");
			}
			ImGui::Separator();
			{
				ImGui::Text("Mie");
				ImGui::SliderFloat("Mie AbsorptionBase",&component->params.mieAbsorptionBase, 0.0f, 10.0f, "%.3f");
				ImGui::SliderFloat("Mie ScatteringBase",&component->params.mieScatteringBase, 0.0f, 10.0f, "%.3f");
				ImGui::SliderFloat("Mie Altitude", &component->params.mieAltitude, 0.0f, 10.0f, "%.3f");
			}
			ImGui::Separator();

		},true);


		
	}
}
