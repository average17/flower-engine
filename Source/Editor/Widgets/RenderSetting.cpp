#include "Pch.h"
#include "RenderSetting.h"
#include "../Editor.h"
#include "DrawComponent.h"
#include "ViewportCamera.h"

using namespace Flower;
using namespace Flower::UI;

static const std::string DETAIL_RenderSettinglIcon = ICON_FA_GEAR;

WidgetRenderSetting::WidgetRenderSetting()
	: Widget("  " + DETAIL_RenderSettinglIcon + "  RenderSetting")
{

}

WidgetRenderSetting::~WidgetRenderSetting() noexcept
{

}

void WidgetRenderSetting::onInit()
{

}

void WidgetRenderSetting::onRelease()
{

}

void WidgetRenderSetting::onTick(const RuntimeModuleTickData& tickData)
{
	m_viewportCamera = GEditor->getWidgetViewport()->getCamera();
}


void WidgetRenderSetting::onVisibleTick(const RuntimeModuleTickData& tickData)
{
	ImGui::Spacing();

	ImGui::TextDisabled("Global render setting for flower engine.");

	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::CollapsingHeader("Bloom Setting"))
	{
		ImGui::PushID("BloomSetting");
		ImGui::Spacing();
		ImGui::Indent();
		ImGui::PushItemWidth(100.0f);

		ImGui::DragFloat("Intensity", &RenderSettingManager::get()->bloomIntensity, 0.01f, 0.0f, 1.0);
		ImGui::DragFloat("Radius", &RenderSettingManager::get()->bloomRadius, 0.01f, 0.0f, 1.0);

		ImGui::DragFloat("Threshold", &RenderSettingManager::get()->bloomThreshold, 0.01f, 0.0f, 1.0);
		ImGui::DragFloat("Threshold soft", &RenderSettingManager::get()->bloomThresholdSoft, 0.01f, 0.0f, 1.0);

		ImGui::PopItemWidth();
		ImGui::Unindent();
		ImGui::Spacing();
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader("Exposure setting"))
	{
		ImGui::PushID("ExposureSetting");
		ImGui::Spacing();
		ImGui::Indent();
		ImGui::PushItemWidth(100.0f);


		ImGui::DragFloat("Low Percent", &RenderSettingManager::get()->AUTOEXPOSURE_lowPercent, 1e-3f, 0.01f, 0.5f);
		ImGui::DragFloat("High Percent", &RenderSettingManager::get()->AUTOEXPOSURE_highPercent, 1e-3f, 0.5f, 0.99f);

		ImGui::DragFloat("Min Brightness", &RenderSettingManager::get()->AUTOEXPOSURE_minBrightness, 0.5f, -9.0f, 9.0f);
		ImGui::DragFloat("Max Brightness", &RenderSettingManager::get()->AUTOEXPOSURE_maxBrightness, 0.5f, -9.0f, 9.0f);

		ImGui::DragFloat("Speed Down", &RenderSettingManager::get()->AUTOEXPOSURE_speedDown, 0.1f, 0.0f);
		ImGui::DragFloat("Speed Up", &RenderSettingManager::get()->AUTOEXPOSURE_speedUp, 0.1f, 0.0f);

		ImGui::DragFloat("Exposure Compensation", &RenderSettingManager::get()->AUTOEXPOSURE_exposureCompensation, 1.0f, 0.0f);

		ImGui::PopItemWidth();
		ImGui::Unindent();
		ImGui::Spacing();
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader("IBL Setting"))
	{
		ImGui::Spacing();
		ImGui::Indent();
		ImGui::PushItemWidth(100.0f);


		ImGui::Checkbox("IBL Lighting", &RenderSettingManager::get()->ibl.bEnableIBLLight);

		if (!RenderSettingManager::get()->ibl.bEnableIBLLight)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button("Select IBL texture"))
		{
			ImGui::OpenPopup("IBLSelectPopup");
			
		}

		if (ImGui::BeginPopup("IBLSelectPopup"))
		{
			const auto& map = AssetRegistryManager::get()->getTypeAssetSetMap();
			if (map.contains(size_t(EAssetType::Texture)))
			{
				const auto& texMap = map.at(size_t(EAssetType::Texture));
				for (const auto& texId : texMap)
				{
					std::shared_ptr<ImageAssetHeader> tex = std::dynamic_pointer_cast<ImageAssetHeader>(AssetRegistryManager::get()->getHeaderMap().at(texId));
					if (tex->isHdr())
					{
						if (ImGui::MenuItem(tex->getName().c_str()))
						{
							auto nexSrc = TextureManager::get()->getOrCreateImage(tex);
							if (RenderSettingManager::get()->ibl.hdrSrc != nexSrc)
							{
								RenderSettingManager::get()->ibl.hdrSrc = nexSrc;
								RenderSettingManager::get()->ibl.setDirty(true);
							}
							
						}
					}
				}
			}
			ImGui::EndPopup();
		}

		ImGui::DragFloat("Intensity", &RenderSettingManager::get()->ibl.intensity, 0.01f, 0.0f, 1.0);

		if (!RenderSettingManager::get()->ibl.bEnableIBLLight)
		{
			ImGui::EndDisabled();
		}

		ImGui::PopItemWidth();
		ImGui::Unindent();
		ImGui::Spacing();
	}

	if (ImGui::CollapsingHeader("GTAO Setting"))
	{
		ImGui::Spacing();
		ImGui::Indent();
		ImGui::PushItemWidth(100.0f);

		ImGui::DragInt("slice num", &RenderSettingManager::get()->GTAO_sliceNum, 1, 1, 8);
		if (RenderSettingManager::get()->GTAO_sliceNum >= 1)
		{
			RenderSettingManager::get()->GTAO_sliceNum = (int)glm::clamp(getNextPOT(RenderSettingManager::get()->GTAO_sliceNum), 1u, 8u);
		}
		else
		{
			RenderSettingManager::get()->GTAO_sliceNum = 1;
		}

		ImGui::DragInt("step times", &RenderSettingManager::get()->GTAO_stepNum, 1, 1, 12);
		ImGui::DragFloat("radius", &RenderSettingManager::get()->GTAO_radius, 0.1f, 0.5f, 4.0f);
		ImGui::DragFloat("thickness", &RenderSettingManager::get()->GTAO_thickness, 0.1f, 0.1f, 1.0f);

		ImGui::DragFloat("ao power", &RenderSettingManager::get()->GTAO_Power, 0.1f, 0.1f, 3.0f);
		ImGui::DragFloat("ao intensity", &RenderSettingManager::get()->GTAO_Intensity, 0.1f, 0.0f, 1.0f);

		ImGui::PopItemWidth();
		ImGui::Unindent();
		ImGui::Spacing();
	}

	if (ImGui::CollapsingHeader("Viewport Camera Control"))
	{
		ImGui::Spacing();
		ImGui::Indent();
		ImGui::PushItemWidth(100.0f);
		ImGui::DragFloat("Camera speed min", &m_viewportCamera->m_minMouseMoveSpeed, 1.0f, 1.0f, m_viewportCamera->m_maxMouseMoveSpeed);

		ImGui::DragFloat("Camera speed max", &m_viewportCamera->m_maxMouseMoveSpeed, 1.0f, m_viewportCamera->m_minMouseMoveSpeed, 1000.0f);

		ImGui::DragFloat("Camera speed", &m_viewportCamera->m_moveSpeed, 1.0f, m_viewportCamera->m_minMouseMoveSpeed, m_viewportCamera->m_maxMouseMoveSpeed);

		ImGui::DragFloat("aperture", &m_viewportCamera->aperture, 1.0f, 1.0f, 100.0f);
		ImGui::DragFloat("ISO", &m_viewportCamera->iso, 1.0f, 50.0f, 1000.0f);
		ImGui::DragFloat("shutter speed", &m_viewportCamera->shutterSpeed, 0.01f, 1.0f / 360.0f, 20.0f);
		ImGui::DragFloat("exposure compensation", &m_viewportCamera->exposureCompensation, 1.0f, -100.0f, 100.0f);

		ImGui::PopItemWidth();
		ImGui::Unindent();
		ImGui::Spacing();
	}

	if (ImGui::CollapsingHeader("Global Tonemmaper Setting"))
	{
		ImGui::Spacing();
		ImGui::Indent();
		ImGui::PushItemWidth(100.0f);

#if 0
		const char* listbox_items[] = { "Campfire", "Lamp", "P3_1000_nits", "Rec2020_1000_nits", "Rec709_5000_nits"};
		int listbox_item_current = int(RenderSettingManager::get()->lummapCase);
		ImGui::ListBox("Case", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items), 5);

		RenderSettingManager::get()->lummapCase = LummapCase(listbox_item_current);
#endif


		bool bHDR = RenderSettingManager::get()->displayMode == RHI::DISPLAYMODE_HDR10_2084;
		ImGui::Checkbox("Hdr10_2084", &bHDR);

		RenderSettingManager::get()->displayMode = bHDR ? RHI::DISPLAYMODE_HDR10_2084 : RHI::DISPLAYMODE_SDR;

		ImGui::DragFloat("Max brightness", &RenderSettingManager::get()->tonemapper_P, 1.0f, 1.0f);

		if (bHDR)
		{
			ImGui::DragFloat("scale", &RenderSettingManager::get()->tonemmaper_s, 1.0f, 1.0f);
		}

		ImGui::DragFloat("contrast", &RenderSettingManager::get()->tonemapper_a, 1.0f, 1.0f);
		ImGui::DragFloat("linear section start", &RenderSettingManager::get()->tonemapper_m, 1.0f, 1.0f);
		ImGui::DragFloat("linear section length", &RenderSettingManager::get()->tonemapper_l, 1.0f, 1.0f);
		ImGui::DragFloat("black", &RenderSettingManager::get()->tonemapper_c, 1.0f, 1.0f);
		ImGui::DragFloat("pedestal", &RenderSettingManager::get()->tonemapper_b, 1.0f, 1.0f);

		

		ImGui::PopItemWidth();
		ImGui::Unindent();
		ImGui::Spacing();
	}

	if (ImGui::CollapsingHeader("Atmosphere Config"))
	{
		ImGui::Indent();
		ImGui::PushItemWidth(100.0f);

		ImGui::Text("Camera config");
		ImGui::Spacing();
		{
			ImGui::DragFloat("Height offset(km)", &m_viewportCamera->atmosphereHeightOffset, 0.1f, 0.0f);

			ImGui::DragFloat("Position scale(m)", &m_viewportCamera->atmosphereMoveScale, 1.0f, 1.0f);
		}
		
		ImGui::Separator();
		ImGui::Spacing();

		auto* comp = &RenderSettingManager::get()->earthAtmosphere;
		auto& earthAtmosphere = comp->earthAtmosphere;

		if (ImGui::CollapsingHeader("Earth Atmosphere"))
		{
			ImGui::PushItemWidth(180.0f);
			ImGui::DragFloat("PreExposure", &earthAtmosphere.atmospherePreExposure, 0.01f, 0.01f, 1.0f);
			ImGui::DragFloat("Mie phase", &earthAtmosphere.miePhaseFunctionG, 0.0f, 0.999f);

			ImGui::ColorEdit3("MieScattCoeff", &comp->mieScatteringColor.x);
			ImGui::DragFloat("MieScattScale", &comp->mieScatteringLength, 0.00001f, 0.1f);
			ImGui::ColorEdit3("MieAbsorCoeff", &comp->mieAbsColor.x);
			ImGui::DragFloat("MieAbsorScale", &comp->mieAbsLength, 0.00001f, 10.0f);
			ImGui::ColorEdit3("RayScattCoeff", &comp->rayleighScatteringColor.x);
			ImGui::DragFloat("RayScattScale", &comp->rayleighScatteringLength, 0.00001f, 10.0f);
			ImGui::ColorEdit3("AbsorptiCoeff", &comp->absorptionColor.x);
			ImGui::DragFloat("AbsorptiScale", &comp->absorptionLength, 0.00001f, 10.0f);
			ImGui::DragFloat("Planet radius", &earthAtmosphere.bottomRadius, 100.0f, 8000.0f);
			ImGui::DragFloat("Atmos height", &comp->atmosphereHeight, 10.0f, 150.0f);
			ImGui::DragFloat("MieScaleHeight", &comp->mieScaleHeight, 0.5f, 20.0f);
			ImGui::DragFloat("RayScaleHeight", &comp->rayleighScaleHeight, 0.5f, 20.0f);

			ImGui::ColorEdit3("Ground albedo", &comp->groundAbledo.x);

			{
				int minVal = earthAtmosphere.viewRayMarchMinSPP;
				int maxSample = earthAtmosphere.viewRayMarchMaxSPP;
				ImGui::SliderInt("Min SPP", &minVal, 1, 30);
				ImGui::SliderInt("Max SPP", &maxSample, 2, 31);

				earthAtmosphere.viewRayMarchMinSPP = minVal;
				earthAtmosphere.viewRayMarchMaxSPP = maxSample;
			}


			ImGui::PopItemWidth();

			earthAtmosphere.mieScattering = comp->mieScatteringColor * comp->mieScatteringLength;
			earthAtmosphere.mieExtinction = earthAtmosphere.mieScattering + comp->mieAbsColor * comp->mieAbsLength;

			earthAtmosphere.rayleighScattering = comp->rayleighScatteringColor * comp->rayleighScatteringLength;
			earthAtmosphere.absorptionExtinction = comp->absorptionColor * comp->absorptionLength;
			earthAtmosphere.topRadius = earthAtmosphere.bottomRadius + comp->atmosphereHeight;
			earthAtmosphere.groundAlbedo = comp->groundAbledo;

			comp->mieDensityProfile.layers[1].expScale = -1.0f / comp->mieScaleHeight;
			comp->rayleighDensityProfile.layers[1].expScale = -1.0f / comp->rayleighScaleHeight;

			memcpy(earthAtmosphere.rayleighDensity, &comp->rayleighDensityProfile, sizeof(comp->rayleighDensityProfile));
			memcpy(earthAtmosphere.mieDensity, &comp->mieDensityProfile, sizeof(comp->mieDensityProfile));
			memcpy(earthAtmosphere.absorptionDensity, &comp->absorptionDensityProfile, sizeof(comp->absorptionDensityProfile));
		}

		if (ImGui::CollapsingHeader("Earth Cloud"))
		{
			ImGui::PushItemWidth(180.0f);

			auto& earthAtmosphere = comp->earthAtmosphere;

			ImGui::DragFloat("Start height", &comp->cloudBottomAltitude, 0.1f, 0.0f, 20.0f);
			ImGui::DragFloat("Thickness", &comp->cloudHeight, 0.1f, 0.1f, 20.0f);

			earthAtmosphere.cloudAreaStartHeight = comp->cloudBottomAltitude + earthAtmosphere.bottomRadius;
			earthAtmosphere.cloudAreaThickness = comp->cloudHeight;

			ImGui::PopItemWidth();
		}

		

		ImGui::PopItemWidth();

		ImGui::Unindent();
	}
}