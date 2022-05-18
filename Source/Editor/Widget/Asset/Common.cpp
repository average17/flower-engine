#include "Common.h"

bool EntryInfoCompare::operator()(const EntryInfo& a,const EntryInfo& b)
{
	if ((a.bFolder && b.bFolder) || (!a.bFolder && !b.bFolder))
	{
		return a.filenameString < b.filenameString;
	}
	return a.bFolder > b.bFolder;
}

void ViewItemDetail::updateRect()
{
    ImGuiStyle& style = ImGui::GetStyle();

    rectButton = ImRect(
        ImGui::GetCursorScreenPos().x,
        ImGui::GetCursorScreenPos().y,
        ImGui::GetCursorScreenPos().x + thumbnailSize,
        ImGui::GetCursorScreenPos().y + thumbnailSize
    );

    rectLabel = ImRect(
        rectButton.Min.x,
        rectButton.Max.y,
        rectButton.Max.x,
        rectButton.Max.y + labelHeight * 3.0f
    );

}

void ViewItemDetail::updateBasic()
{
    labelHeight   = GImGui->FontSize * 1.2f;
    buttomOffset  = ImGui::GetTextLineHeight() * 1.2f;

    contentWidth  = ImGui::GetContentRegionAvail().x;
    contentHeight = ImGui::GetContentRegionAvail().y - buttomOffset;
}