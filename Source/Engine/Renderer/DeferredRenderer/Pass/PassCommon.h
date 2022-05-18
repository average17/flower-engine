#pragma once
#include "../DefferedRenderer.h"

namespace flower
{
	inline uint32_t getGroupCount(uint32_t threadCount, uint32_t localSize)
	{
		return (threadCount + localSize - 1) / localSize;
	}

	class GraphicsPass
	{
	public:
		GraphicsPass() = delete;
		GraphicsPass(DefferedRenderer* renderer, const std::string& name);

		virtual ~GraphicsPass();

		void init();
		void release();

		DefferedRenderer* getRenderer() const { return m_renderer; }
		std::string getPassName() const { return m_passName; }

	private:
		std::string m_passName;

	protected:
		DefferedRenderer* m_renderer = nullptr;

		virtual void initInner() = 0;
		virtual void onSceneTextureRecreate(uint32_t width, uint32_t height) = 0;
		virtual void releaseInner() = 0;
	};
}