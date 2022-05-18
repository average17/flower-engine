#pragma once
#include "../Core/RuntimeModule.h"
#include "../Core/Delegates.h"

#include "ImGuiPass.h"
#include "RenderTexturePool.h"

namespace flower
{
	constexpr uint32_t MIN_RENDER_SIZE = 64;

	// for renderer module.
	// you should always add uisystem module.

	// simple renderer interface.
	class Renderer : public IRuntimeModule
	{
	public:
		Renderer(ModuleManager* in, std::string name = "Renderer");
		virtual ~Renderer() {  }

		virtual void registerCheck() override;

		virtual bool init() override;
		virtual void release() override;

		// i offer a simple tick function here for reference.
		// you can override if you need.
		virtual void tick(const TickData&) override;

	protected:
		std::unique_ptr<ImguiPass> m_ui = nullptr;
	};

	class OffscreenRenderer: public Renderer
	{
	public:
		OffscreenRenderer(ModuleManager* in, std::string name = "OffscreenRenderer");
		virtual ~OffscreenRenderer() {  }

		virtual bool init() override;
		virtual void release() override;

		uint32_t getRenderWidth() const { return m_offscreenRenderWidth; }
		uint32_t getRenderHeight() const { return m_offscreenRenderHeight; }

	protected:
		uint32_t m_offscreenRenderWidth  = MIN_RENDER_SIZE;
		uint32_t m_offscreenRenderHeight = MIN_RENDER_SIZE;

		// common dynamic command buffer meaning it update every frame.
		std::array<VulkanCommandBuffer*, MAX_FRAMES_IN_FLIGHT> m_dynamicGraphicsCommandBuffer;
		std::array<VkSemaphore,MAX_FRAMES_IN_FLIGHT> m_dynamicGraphicsCommandExecuteSemaphores;

		// async compute.

	};


	struct RenderStateMonitor
	{
		DelegatesSingleThread<RenderStateMonitor> callbacks;

		void broadcast() { callbacks.broadcast(); }
	};

	extern RenderStateMonitor GRenderStateMonitor;
}