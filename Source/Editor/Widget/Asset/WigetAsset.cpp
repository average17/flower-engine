#include "WigetAsset.h"

#include <Engine/Core/UtilsPath.h>
#include <Engine/Async/AsyncThread.h>
#include <Engine/Core/Core.h>
#include <Engine/Asset/AssetSystem.h>
#include <ImGui/ImGui.h>
#include <ImGui/IconsFontAwesome5.h>
#include <Engine/Engine.h>

using namespace flower;

std::string ASSET_TILE_ICON = ICON_FA_FOLDER_OPEN;

WidgetAsset::WidgetAsset(flower::Engine* engine)
	: Widget(engine," " + ASSET_TILE_ICON + "  Asset")
{
	m_assetSystem = engine->getRuntimeModule<AssetSystem>();

	// todo: configable.
	std::string projectDir = UtilsPath::getUtilsPath()->getProjectPath();
	
	setProjectDir(projectDir);
}

WidgetAsset::~WidgetAsset() noexcept
{
	
}

void WidgetAsset::setProjectDir(std::string projectDir)
{
	CHECK(std::filesystem::exists(projectDir));

	m_projectDir = projectDir;

	// also need to update content view directory.
	m_contentView.init(projectDir, this);
}

void WidgetAsset::onInit()
{
	if(AssetSystem::onProjectDirDirty.registerFunction(m_onProjectDirtyCallbackName,[this]()
	{
		m_contentView.setDirty();
	}))
	{
		LOG_TRACE("register callback {}.", m_onProjectDirtyCallbackName);
	}
	else
	{
		LOG_WARN("fail to register callback {}, maybe exist some error.", m_onProjectDirtyCallbackName);
	}
}

void WidgetAsset::onTick(const flower::TickData& tickData,size_t frameIndex)
{
	
}

void WidgetAsset::onRelease()
{
	if(AssetSystem::onProjectDirDirty.unregisterFunction(m_onProjectDirtyCallbackName))
	{
		LOG_TRACE("unregister callback {}.", m_onProjectDirtyCallbackName);
	}
	else
	{
		LOG_WARN("fail to unregister callback {}, maybe exist some error.", m_onProjectDirtyCallbackName);
	}
}

void WidgetAsset::onVisibleTick(const flower::TickData& tickData,size_t)
{
	// inspect cache path entries.

	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);

	
	if (!ImGui::Begin(getTile().c_str(),&m_bShow, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::End();
		return;
	}

	if (ImGui::BeginTable("AssetContentTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		{
			ImGui::BeginChild("TreeViewContent", ImVec2(0.0f,0.0f), true, ImGuiWindowFlags_NoMove);
			{
				// draw tree view.
				m_contentView.drawTree();
			}
			ImGui::EndChild();
		}

		ImGui::TableSetColumnIndex(1);
		{
			ImGui::BeginGroup();
			{
				ImGui::Spacing();

				ImGui::BeginChild("ContentViewMisc", ImVec2(0.0f,0.0f), true, ImGuiWindowFlags_NoMove);
				{
					// draw content view.
					m_contentView.drawContent();
				}
				ImGui::EndChild();
			}
			ImGui::EndGroup();
		}

		ImGui::EndTable();
	}

	ImGui::End();
}