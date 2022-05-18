#include "DefferedRenderer.h"
#include "../ImGuiPass.h"
#include "Pass/GBufferPass.h"
#include "Pass/LightingPass.h"
#include "Pass/TonemapperPass.h"
#include "Pass/CullingPass.h"
#include "Pass/SDSMPass.h"
#include "Pass/TAAPass.h"
#include "Pass/BRDFLut.h"
#include "Pass/SpecularCaptureFilter.h"
#include "../../Engine.h"
#include "../../Asset/AssetSystem.h"
#include "../../Asset/AssetTexture.h"
#include "Pass/Bloom.h"
#include "Pass/HizBuild.h"
#include "Pass/SSSR.h"
#include "Pass/BlueNoise.h"
#include "Pass/SSAO.h"
#include "Pass/EpicAtmosphere.h"

namespace flower
{
	DefferedRenderer::DefferedRenderer(ModuleManager* in,std::string name)
		: OffscreenRenderer(in, name)
	{
	}

	VulkanImage* DefferedRenderer::getOutputImage()
	{
		return m_sceneTextures.getLdrSceneColor();
	}

	void DefferedRenderer::onRenderSizeChange(bool bForceRebuild)
	{
		// first release all scene textures.
		m_sceneTextures.updateSize(getRenderWidth(), getRenderHeight(), bForceRebuild);

		// then broadcast all pass's rebuild functions.
		m_renderSizeChangeCallback.broadcast(getRenderWidth(), getRenderHeight());

		m_renderTimes = 0;
	}

	bool DefferedRenderer::updateRenderSize(uint32_t width,uint32_t height)
	{
		bool bRebuild = false;
		if(m_offscreenRenderWidth != width || m_offscreenRenderHeight != height)
		{
			m_offscreenRenderWidth  = std::max(MIN_RENDER_SIZE, width);
			m_offscreenRenderHeight = std::max(MIN_RENDER_SIZE, height);

			onRenderSizeChange(false);

			bRebuild = true;
			
		}

		return bRebuild;
	}

	bool DefferedRenderer::init()
	{
		m_textureManager = GEngine.getRuntimeModule<AssetSystem>()->getTextureManager();

		bool result = OffscreenRenderer::init();
		{
			GRenderStateMonitor.callbacks.registerFunction("DefferredRenderStateMonitor", [this]() {
				onRenderSizeChange(true);
			});

			m_frameDataRing.init();
			m_viewDataRing.init();

			m_materialDataRing.init();
			m_objectDataRing.init();

			m_renderScene.init();

			m_drawIndirectSSBOGbuffer.init();

			m_sceneTextures.updateSize(getRenderWidth(), getRenderHeight(), false);

			{
				BRDFLutPass* brdfLut = new BRDFLutPass(this);
				brdfLut->init();
				brdfLut->update();
				brdfLut->release();
				delete brdfLut;
			}

			m_specularCapturePass = new SpecularCaptureFilterPass(this);
			m_specularCapturePass->init();

			// filter image for fallback.
			m_specularCapturePass->filterImage(this->m_staticTextures.getFallbackReflectionCapture(),
				m_textureManager->getEngineImage(engineImages::fallbackCapture));
			
			m_blueNoise = new BlueNoisePass(this);
			m_blueNoise->init();

			m_epicAtmospherePass = new EpicAtmospherePass(this);
			m_epicAtmospherePass->init();

			m_cullingPass = new CullingPass(this);
			m_cullingPass->init();

			m_gbufferPass = new GBufferPass(this);
			m_gbufferPass->init();

			m_hizPass = new HizPass(this);
			m_hizPass->init();

			m_sdsmPass = new SDSMPass(this);
			m_sdsmPass->init();

			m_sssrPass = new SSSR(this);
			m_sssrPass->init();

			m_ssaoPass = new SSAO(this);
			m_ssaoPass->init();

			m_lightingPass = new LightingPass(this);
			m_lightingPass->init();

			m_taaPass = new TAAPass(this);
			m_taaPass->init();

			m_bloomPass = new Bloom(this);
			m_bloomPass->init();

			m_tonemapperPass = new TonemapperPass(this);
			m_tonemapperPass->init();
		}
		return result;
	}

	void DefferedRenderer::tick(const TickData& tickData)
	{
		// do nothing if minimized.
		if(tickData.bIsMinimized)
		{
			return;
		}

		// update render scene data.
		m_renderScene.tick(tickData);

		uint32_t backBufferIndex = RHI::get()->acquireNextPresentImage();
		CHECK(backBufferIndex < MAX_FRAMES_IN_FLIGHT && "Swapchain backbuffer count should equal to flighting count.");
		auto* offscreenCmd = m_dynamicGraphicsCommandBuffer[backBufferIndex];

		// NOTE: we use immediate present mode, so it's very important to sync after main fence finish.
		// if sync before main fence.
		// maybe there exist more than 3 buffer, so data late.
		{
			// current frame static uniform buffer prepare.
			m_frameDataRing.onFrameBegin();
			m_viewDataRing.onFrameBegin();
			m_objectDataRing.onFrameBegin();
			m_materialDataRing.onFrameBegin();

			// update gpu static buffer.
			m_frameDataRing.updateData(m_renderScene.getFrameData());
			m_viewDataRing.updateData(m_renderScene.getViewData());

			// upload mesh object ssbo.
			{
				size_t objBoundSize = sizeof(GPUObjectData) * m_renderScene.getMeshObjectSSBOData().size();
				if (objBoundSize > 0)
				{
					m_objectDataRing.getBuffer()->map();
					m_objectDataRing.getBuffer()->copyTo(m_renderScene.getMeshObjectSSBOData().data(), objBoundSize);
					m_objectDataRing.getBuffer()->unmap();
				}
			}

			// upload mesh material ssbo.
			{
				size_t materialBoundSize = sizeof(GPUMaterialData) * m_renderScene.getMeshMaterialSSBOData().size();
				if (materialBoundSize > 0)
				{
					m_materialDataRing.getBuffer()->map();
					m_materialDataRing.getBuffer()->copyTo(m_renderScene.getMeshMaterialSSBOData().data(), materialBoundSize);
					m_materialDataRing.getBuffer()->unmap();
				}
			}
		}

		vkCheck(vkResetCommandBuffer(offscreenCmd->getInstance(),0));
		VkCommandBufferBeginInfo cmdBeginInfo = vkCommandbufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		vkCheck(vkBeginCommandBuffer(offscreenCmd->getInstance(),&cmdBeginInfo));
		{
			m_blueNoise->dynamicRender(offscreenCmd->getInstance());

			m_epicAtmospherePass->prepareSkyView(offscreenCmd->getInstance());
			

			m_cullingPass->gbufferCulling(offscreenCmd->getInstance());
			m_gbufferPass->dynamicRender(offscreenCmd->getInstance());
			m_hizPass->dynamicRender(offscreenCmd->getInstance());
			
			m_sdsmPass->dynamicRecord(offscreenCmd->getInstance());

			// todo: async.
			m_sssrPass->dynamicRender(offscreenCmd->getInstance());
			m_ssaoPass->dynamicRender(offscreenCmd->getInstance());


			m_lightingPass->dynamicRender(offscreenCmd->getInstance());

			m_epicAtmospherePass->drawSky(offscreenCmd->getInstance());

			
			m_taaPass->dynamicRender(offscreenCmd->getInstance());
			m_bloomPass->dynamicRender(offscreenCmd->getInstance());
			
			m_tonemapperPass->dynamicRender(offscreenCmd->getInstance());
		}
		vkCheck(vkEndCommandBuffer(offscreenCmd->getInstance()));

		// UI rendering.
		{
			m_ui->renderFrame(backBufferIndex);

			auto frameStartSemaphore = RHI::get()->getCurrentFrameWaitSemaphoreRef();
			auto frameEndSemaphore   = RHI::get()->getCurrentFrameFinishSemaphore();

			std::vector<VkSemaphore> offscreenWaitSemaphores = 
			{ 
				frameStartSemaphore 
			};
			std::vector<VkPipelineStageFlags> offscreenWaitFlags = 
			{ 
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			};

			auto* offscreenSemaphore = &m_dynamicGraphicsCommandExecuteSemaphores[backBufferIndex];

			VulkanSubmitInfo offscreenSubmitInfo{};
			offscreenSubmitInfo.setWaitStage(offscreenWaitFlags)
				.setWaitSemaphore(offscreenWaitSemaphores.data(), (int32_t)offscreenWaitSemaphores.size())
				.setSignalSemaphore(offscreenSemaphore, 1)
				.setCommandBuffer(offscreenCmd, 1);
			
			std::vector<VkSemaphore> uiWaitSemaphores = 
			{ 
				*offscreenSemaphore 
			};
			std::vector<VkPipelineStageFlags> uiWaitFlags = 
			{ 
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			};
			VulkanSubmitInfo imguiPassSubmitInfo{};
			VkCommandBuffer cmd_uiPass = m_ui->getCommandBuffer(backBufferIndex);
			imguiPassSubmitInfo.setWaitStage(uiWaitFlags)
				.setWaitSemaphore(uiWaitSemaphores.data(), (int32_t)uiWaitSemaphores.size())
				.setSignalSemaphore(frameEndSemaphore,1)
				.setCommandBuffer(&cmd_uiPass,1);

			std::vector<VkSubmitInfo> submitInfos = { offscreenSubmitInfo, imguiPassSubmitInfo };

			RHI::get()->resetFence();
			RHI::get()->submit((uint32_t)submitInfos.size(), submitInfos.data());
			ImGuiCommon::updateAfterSubmit();
		}

		RHI::get()->present();
		m_renderTimes++;

		if (m_renderTimes >= 0xFFFFFFFF)
		{
			m_renderTimes = 0;
		}
	}

	void DefferedRenderer::release()
	{
		GRenderStateMonitor.callbacks.unregisterFunction("DefferredRenderStateMonitor");
		// self release.
		{
			m_frameDataRing.release();
			m_viewDataRing.release();

			m_sceneTextures.release();
			m_staticTextures.release();
			m_objectDataRing.release();
			m_materialDataRing.release();
			m_drawIndirectSSBOGbuffer.release();

			m_blueNoise->release();
			delete m_blueNoise;

			m_specularCapturePass->release();
			delete m_specularCapturePass;

			m_epicAtmospherePass->release();
			delete m_epicAtmospherePass;

			m_cullingPass->release();
			delete m_cullingPass;

			m_gbufferPass->release();
			delete m_gbufferPass;

			m_hizPass->release();
			delete m_hizPass;

			m_sdsmPass->release();
			delete m_sdsmPass;

			m_lightingPass->release();
			delete m_lightingPass;

			m_sssrPass->release();
			delete m_sssrPass;

			m_ssaoPass->release();
			delete m_ssaoPass;

			m_taaPass->release();
			delete m_taaPass;

			m_bloomPass->release();
			delete m_bloomPass;

			m_tonemapperPass->release();
			delete m_tonemapperPass;
		}
		OffscreenRenderer::release();
	}
}