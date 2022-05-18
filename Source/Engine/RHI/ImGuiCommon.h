#pragma once

namespace flower
{
	class ImGuiCommon
	{
	public:
		static void init();
		static void release();

		static void newFrame();
		static void updateAfterSubmit();

	private:
		static void setupStyle();
		

		inline static const float m_fontSize    = 15.0f;
		inline static const float m_iconSize    = 13.0f;
		inline static const float m_fontScale   = 1.0f;
	};
}