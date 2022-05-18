#include "WidgetViewport.h"

#include <Engine/Core/Core.h>
#include <Engine/RHI/RHI.h>
#include <Engine/Engine.h>
#include <Engine/Renderer/Renderer.h>
#include <Engine/Renderer/DeferredRenderer/DefferedRenderer.h>
#include <Engine/Core/WindowData.h>
#include <glfw/glfw3.h>
#include <ImGui/IconsFontAwesome5.h>

using namespace flower;

std::string VIEWPORT_TILE_ICON = ICON_FA_EYE;

WidgetViewport::WidgetViewport(flower::Engine* engine)
	: Widget(engine, " " + VIEWPORT_TILE_ICON + "  Viewport")
{
	m_renderer = engine->getRuntimeModule<DefferedRenderer>();
}

WidgetViewport::~WidgetViewport() noexcept
{

}

void WidgetViewport::onInit()
{
	GRenderStateMonitor.callbacks.registerFunction("WidgetViewportRenderStateMonitor", [this](){
		m_bNeedRebuild = true;
	});
}

void WidgetViewport::onTick(const flower::TickData& tickData,size_t)
{

}

void WidgetViewport::onRelease()
{
	GRenderStateMonitor.callbacks.unregisterFunction("WidgetViewportRenderStateMonitor");
}

void WidgetViewport::onVisibleTick(const flower::TickData& tickData,size_t frameIndex)
{
	int32_t glfwWidth = 0, glfwHeight = 0;
	glfwGetFramebufferSize(GLFWWindowData::get()->getWindow(), &glfwWidth,&glfwHeight);
	bool bMiniWindow = (glfwWidth == 0 || glfwHeight == 0);

	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
	if(!bMiniWindow)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
	}

	if (!ImGui::Begin(getTile().c_str(),&m_bShow, ImGuiWindowFlags_NoTitleBar))
	{
		if (!bMiniWindow)
		{
			ImGui::PopStyleVar(1);
		}
		ImGui::End();
		return;
	}

	float width   = ImGui::GetContentRegionAvail().x;
	float height  = ImGui::GetContentRegionAvail().y;

	uint32_t safeWidth  = std::max(MIN_RENDER_SIZE, (uint32_t)width);
	uint32_t safeHeight = std::max(MIN_RENDER_SIZE, (uint32_t)height);

	bool bRebuild = m_bNeedRebuild;
	m_bNeedRebuild = false;
	bRebuild |= m_renderer->updateRenderSize(safeWidth, safeHeight);

	m_cacheImageInfo.setIcon(m_renderer->getOutputImage());
	m_cacheImageInfo.setSampler(RHI::get()->getPointClampEdgeSampler());
	
	ImGui::Image(m_cacheImageInfo.getId(uint32_t(frameIndex), bRebuild), ImVec2(width,height));
	GLFWWindowData::get()->setMouseInViewport(ImGui::IsItemHovered());

	ImGui::End();

	if(!bMiniWindow)
	{
		ImGui::PopStyleVar(1);
	}
}
