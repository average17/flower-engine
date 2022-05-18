#include "ImGuiCommon.h"
#include "../Core/WindowData.h"

#include <ImGui/ImGui.h>
#include <ImGui/ImGuiGlfw.h>
#include <ImGui/ImGuiVulkan.h>
#include <ImGui/ImNodes.h>
#include <ImGui/IconsFontAwesome5.h>

namespace flower
{
	void ImGuiCommon::init()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImNodes::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); 
		(void)io;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		// 
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		io.IniFilename = "Setting/Editor/ImGui.ini";
	
		setupStyle();
		ImGuiStyle& style = ImGui::GetStyle();

		// todo: nodes style customize.
		ImNodes::StyleColorsDark();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForVulkan(GLFWWindowData::get()->getWindow(),true);
	
		// load font data to memory.
		io.Fonts->AddFontFromFileTTF(
			(UtilsPath::getUtilsPath()->getEngineFontFolderPath() + fontAsset::deng).c_str(),
			m_fontSize, 
			NULL, 
			io.Fonts->GetGlyphRangesChineseFull()
		);

		// merge in icons from Font Awesome
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config; 
		icons_config.MergeMode = true; 
		icons_config.PixelSnapH = true;

		io.Fonts->AddFontFromFileTTF(
			(UtilsPath::getUtilsPath()->getEngineFontFolderPath() + FONT_ICON_FILE_NAME_FAS).c_str(),
			m_iconSize,
			&icons_config,
			icons_ranges
		);

		io.Fonts->AddFontFromFileTTF(
			(UtilsPath::getUtilsPath()->getEngineFontFolderPath() + FONT_ICON_FILE_NAME_FAR).c_str(),
			m_iconSize,
			&icons_config,
			icons_ranges
		);

		io.FontGlobalScale = m_fontScale;
	
	}

	void ImGuiCommon::release()
	{
		ImNodes::DestroyContext();

		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiCommon::newFrame()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiCommon::updateAfterSubmit()
	{
		ImGuiIO& io = ImGui::GetIO(); 
		(void)io;

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void ImGuiCommon::setupStyle()
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ChildBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.00f);
		colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
		colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
		colors[ImGuiCol_BorderShadow]           = ImVec4(0.05f, 0.05f, 0.05f, 0.24f);
		colors[ImGuiCol_FrameBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_TitleBg]                = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
		colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
		colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_Button]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_Header]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.52f);
		colors[ImGuiCol_HeaderHovered]          = ImVec4(0.05f, 0.05f, 0.05f, 0.36f);
		colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
		colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_Tab]                    = ImVec4(0.05f, 0.05f, 0.05f, 0.52f);
		colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
		colors[ImGuiCol_TabUnfocused]           = ImVec4(0.05f, 0.05f, 0.05f, 0.52f);
		colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_DockingPreview]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_DockingEmptyBg]         = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.05f, 0.05f, 0.05f, 0.52f);
		colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.05f, 0.05f, 0.05f, 0.52f);
		colors[ImGuiCol_TableBorderLight]       = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
		colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding                     = ImVec2(8.00f, 8.00f);
		style.FramePadding                      = ImVec2(5.00f, 2.00f);
		style.CellPadding                       = ImVec2(6.00f, 6.00f);
		style.ItemSpacing                       = ImVec2(6.00f, 6.00f);
		style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
		style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
		style.IndentSpacing                     = 25;
		style.ScrollbarSize                     = 15;
		style.GrabMinSize                       = 10;
		style.WindowBorderSize                  = 1;
		style.ChildBorderSize                   = 1;
		style.PopupBorderSize                   = 1;
		style.FrameBorderSize                   = 1;
		style.TabBorderSize                     = 1;
		style.WindowRounding                    = 7;
		style.ChildRounding                     = 4;
		style.FrameRounding                     = 3;
		style.PopupRounding                     = 4;
		style.ScrollbarRounding                 = 9;
		style.GrabRounding                      = 3;
		style.LogSliderDeadzone                 = 4;
		style.TabRounding                       = 4;

		style.WindowMenuButtonPosition          = ImGuiDir_None;
	}
}