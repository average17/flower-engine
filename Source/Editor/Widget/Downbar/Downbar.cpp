#include "Downbar.h"

#include <ImGui/ImGui.h>
#include <Engine/Core/WindowData.h>
#include <Engine/Core/Core.h>
#include <Engine/Core/Timer.h>
#include <iomanip>
#include <sstream>
using namespace flower;

WidgetDownbar::WidgetDownbar(flower::Engine* engine)
    : Widget(engine, "Downbar")
{

}

WidgetDownbar::~WidgetDownbar() noexcept
{

}

using TimePoint = std::chrono::system_clock::time_point;
inline std::string downbaarSerializeTimePoint( const TimePoint& time, const std::string& format)
{
	std::time_t tt = std::chrono::system_clock::to_time_t(time);
	std::tm tm = *std::localtime(&tt); 
	std::stringstream ss;
	ss << std::put_time( &tm, format.c_str() );
	return ss.str();
}

void WidgetDownbar::onTick(const TickData& tickData, size_t)
{
	bool hide = true;

	static ImGuiWindowFlags flag =
		ImGuiWindowFlags_NoCollapse         |
		ImGuiWindowFlags_NoResize           |
		ImGuiWindowFlags_NoMove             |
		ImGuiWindowFlags_NoScrollbar        |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoMouseInputs |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavInputs|
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_UnsavedDocument|
		ImGuiWindowFlags_NoTitleBar;

	if (!ImGui::Begin(getTile().c_str(),&hide,flag))
	{
		ImGui::End();
		return;
	}

	std::string fpsText;
	float fps;
	{
		fps = glm::clamp(GTimer.getCurrentSmoothFps(), 0.0f, 999.0f);
		std::stringstream ss;
		ss <<std::setw(4) << std::left << std::setfill(' ') << std::fixed << std::setprecision(0) << fps;
		fpsText = "MainLoop: " + ss.str() + "FPS";
	}

	if(ImGui::BeginDownBar(1.1f))
	{
		// draw engine info
		{
			TimePoint input = std::chrono::system_clock::now();
			std::string name = downbaarSerializeTimePoint(input,"%Y/%m/%d %H:%M:%S");
			ImGui::Text("FlowerEngine@qiutang ");
			ImGui::Text(name.c_str());
		}
		
		const ImVec2 p = ImGui::GetCursorScreenPos();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		float textStartPositionX = 
			ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - ImGui::CalcTextSize(fpsText.c_str()).x 
			- ImGui::GetScrollX() - 2 * ImGui::GetStyle().ItemSpacing.x;

		float sz = ImGui::GetTextLineHeight() * 0.40f;
		float fpsPointX = textStartPositionX - sz * 1.8f;

		glm::vec4 goodColor = {0.0f, 1.0f, 0.0f, 1.0f};
		glm::vec4 badColor = {1.0f,0.0f, 0.0f,1.0f};

		{
			// prepare fps color state
			constexpr float lerpMax = 120.0f;
			constexpr float lerpMin = 30.0f;

			float lerpFps = (glm::clamp(fps, lerpMin, lerpMax) - lerpMin ) / (lerpMax - lerpMin);
			glm::vec4 lerpColorfps = glm::lerp(badColor,goodColor,lerpFps);

			ImVec4 colffps;
			colffps.x = lerpColorfps.x;
			colffps.y = lerpColorfps.y;
			colffps.z = lerpColorfps.z;
			colffps.w = lerpColorfps.w;

			const ImU32 colfps = ImColor(colffps);

			// draw fps state.
			{
				float x = fpsPointX;
				float y = p.y + ImGui::GetFrameHeight() * 0.51f;
				draw_list->AddCircleFilled(ImVec2(x , y ), sz, colfps, 40);

				ImGui::SetCursorPosX(textStartPositionX);
				ImGui::Text("%s", fpsText.c_str());
			}

			
		}
		ImGui::Separator();
		ImGui::EndDownBar();
	}

	ImGui::End();
}