#include "WidgetConsole.h"

#include <Engine/Core/Core.h>
#include <Engine/Engine.h>
#include <Engine/Core/WindowData.h>
#include <ImGui/IconsFontAwesome5.h>

using namespace flower;

std::string CONSOLE_TILE_ICON = ICON_FA_COMMENT;

WidgetConsole::WidgetConsole(flower::Engine* engine)
	: Widget(engine, " " + CONSOLE_TILE_ICON + "  Console")
{
	m_console = std::make_unique<Console>();

	
}

WidgetConsole::~WidgetConsole() noexcept
{
	m_console.reset();
}

void WidgetConsole::onInit()
{
	m_console->init();
}

void WidgetConsole::onTick(const flower::TickData& tickData, size_t)
{
	
}

void WidgetConsole::onRelease()
{
	m_console->release();
}

void WidgetConsole::onVisibleTick(const flower::TickData& tickData, size_t frameIndex)
{
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin(getTile().c_str(), &m_bShow))
	{
		ImGui::End();
		return;
	}

	m_console->draw();

	ImGui::End();
}
