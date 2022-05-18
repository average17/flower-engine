#include "PassCommon.h"

namespace flower
{
	GraphicsPass::GraphicsPass(DefferedRenderer* renderer,const std::string& name)
	: m_renderer(renderer), m_passName(name)
	{

	}

	GraphicsPass::~GraphicsPass()
	{

	}

	void GraphicsPass::init()
	{
		bool bRegisterResult = m_renderer->getRenderSizeChangeCallback()->registerFunction(m_passName + "_onSizeChange",
		[this](uint32_t newWidth,uint32_t newHeight)
		{
			onSceneTextureRecreate(newWidth, newHeight);
		});
		
		CHECK(bRegisterResult && "Fail to register pass size change callback");

		initInner();
	}

	void GraphicsPass::release()
	{
		bool bUnregisterResult = m_renderer->getRenderSizeChangeCallback()->unregisterFunction(m_passName + "_onSizeChange");
		CHECK(bUnregisterResult && "Fail to unregister pass size change callback");

		releaseInner();
	}
}