#include "WidgetCommon.h"
#include <ImGui/ImGui.h>
#include <ImGui/ImGuiInternal.h>
#include <ImGui/IconsFontAwesome5.h>

constexpr const char* RESET_ICON = ICON_FA_REPLY;

WidgetAsset*       WidgetReferences::asset       = nullptr;
WidgetViewport*    WidgetReferences::viewport    = nullptr;
WidgetRenderGraph* WidgetReferences::renderGraph = nullptr;
WidgetHierarhcy*   WidgetReferences::hierarhcy   = nullptr;
WidgetConsole*     WidgetReferences::console     = nullptr;
WidgetInspector*   WidgetReferences::inspector   = nullptr;

void WidgetCommon::drawVector3(
	const std::string& label,
	glm::vec3& values,
	const glm::vec3& resetValue)
{
	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];
	ImGui::PushID(label.c_str());

	
	float size = GImGui->Font->FontSize * label.size() * 2.0f;
	
	ImGui::Columns(2);
	// ImGui::SetCursorPosX(ImGui::GetCursorPosX());
	ImGui::SetColumnWidth(0, size);

	ImGui::Text(label.c_str());


	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

	const float sscale = 0.9f;
	ImVec2 buttonSize = { sscale * (lineHeight + 3.0f), sscale * lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.09f, 0.09f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.8f, 0.09f, 0.1f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.9f, 0.9f, 0.9f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("X", buttonSize))
		values.x = resetValue.x;
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();



	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.08f, 0.2f, 0.1f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.08f, 0.8f, 0.1f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.9f, 0.9f, 0.9f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("Y", buttonSize))
		values.y = resetValue.y;
	ImGui::PopFont();
	ImGui::PopStyleColor(3);



	ImGui::SameLine();
	ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.08f, 0.09f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.08f, 0.09f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.9f, 0.9f, 0.9f, 1.0f });
	ImGui::PushFont(boldFont);
	if (ImGui::Button("Z", buttonSize))
		values.z = resetValue.z;
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");


	ImGui::SameLine();

	if(ImGui::Button(RESET_ICON))
	{
		values.x = resetValue.x;
		values.y = resetValue.y;
		values.z = resetValue.z;
	}

	ImGui::PopItemWidth();
	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();

}