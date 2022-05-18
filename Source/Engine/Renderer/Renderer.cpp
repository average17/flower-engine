#include "Renderer.h"
#include "../UI/UISystem.h"

namespace flower
{
	RenderStateMonitor GRenderStateMonitor;

	Renderer::Renderer(ModuleManager* in, std::string name)
		: IRuntimeModule(in, name)
	{
		m_ui = std::make_unique<ImguiPass>();
	}

	void Renderer::registerCheck()
	{
		// ensure there is uisystem before renderer register.
		CHECK(m_moduleManager->getRuntimeModule<UISystem>() 
			&& "you should register UISystem before you register Renderer Module!");
	}

	bool Renderer::init()
	{
		m_ui->init();
		return true;
	}

	// simple template for other renderer.
	// only render ui then submit and present.
	void Renderer::tick(const TickData& tickData)
	{
		// do nothing if no foucus.
		if(tickData.bIsMinimized || tickData.bLoseFocus)
		{
			return;
		}

		uint32_t backBufferIndex = RHI::get()->acquireNextPresentImage();

		// UI rendering.
		{
			m_ui->renderFrame(backBufferIndex);

			auto frameStartSemaphore = RHI::get()->getCurrentFrameWaitSemaphoreRef();
			auto frameEndSemaphore   = RHI::get()->getCurrentFrameFinishSemaphore();

			std::vector<VkSemaphore> uiWaitSemaphores = 
			{ 
				frameStartSemaphore 
			};

			std::vector<VkPipelineStageFlags> uiWaitFlags = 
			{ 
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			};

			VulkanSubmitInfo imguiPassSubmitInfo{};
			VkCommandBuffer cmd_uiPass = m_ui->getCommandBuffer(backBufferIndex);
			imguiPassSubmitInfo.setWaitStage(uiWaitFlags)
				.setWaitSemaphore(uiWaitSemaphores.data(),(int32_t)uiWaitSemaphores.size())
				.setSignalSemaphore(frameEndSemaphore,1)
				.setCommandBuffer(&cmd_uiPass,1);
			std::vector<VkSubmitInfo> submitInfos = { imguiPassSubmitInfo };

			RHI::get()->resetFence();
			RHI::get()->submit((uint32_t)submitInfos.size(),submitInfos.data());
			ImGuiCommon::updateAfterSubmit();
		}

		RHI::get()->present();
	}

	void Renderer::release()
	{
		m_ui->release();
	}

	OffscreenRenderer::OffscreenRenderer(ModuleManager* in,std::string name)
		: Renderer(in, name)
	{

	}

	bool OffscreenRenderer::init()
	{
		bool result = Renderer::init();

		// prepare common cmdbuffer and semaphore.
		{
			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				m_dynamicGraphicsCommandBuffer[i] = RHI::get()->createGraphicsCommandBuffer();

				CHECK(vkCreateSemaphore(RHI::get()->getDevice(),&semaphoreInfo,nullptr,&m_dynamicGraphicsCommandExecuteSemaphores[i]) == VK_SUCCESS);
			}
		}

		return result;
	}

	void OffscreenRenderer::release()
	{
		// release common cmdbuffer and semaphore.
		{
			for(size_t i = 0; i < m_dynamicGraphicsCommandBuffer.size(); i++)
			{
				delete m_dynamicGraphicsCommandBuffer[i];
				m_dynamicGraphicsCommandBuffer[i] = nullptr;
			}

			for(size_t i = 0; i < m_dynamicGraphicsCommandExecuteSemaphores.size(); i++)
			{
				vkDestroySemaphore(RHI::get()->getDevice(),m_dynamicGraphicsCommandExecuteSemaphores[i],nullptr);
			}
		}

		Renderer::release();
	}
}