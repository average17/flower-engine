#include "ContentViewItem.h"
#include "ContentView.h"
#include "WigetAsset.h"

#include <Engine/Core/Core.h>
#include <Engine/UI/Icon.h>
#include <Engine/UI/UISystem.h>
#include <Engine/RHI/RHI.h>
#include <Engine/Asset/AssetSystem.h>
#include "../../Style/Icon.h"

using namespace flower;

IconInfo* ContentViewItem::prepareIcon(const EntryInfo& info, IconLibrary* lib)
{
    if(info.bFolder)
    {
        return lib->getIconInfo(IconPath::folderIcon);
    }
    else
    {
        auto* assetSystem = m_widget->getAssetSystem();
        if(auto* snapShot = lib->getSnapShots(info.filePath))
        {
            return snapShot;
        }
    }

    return lib->getIconInfo(IconPath::fileIcon);
}

glm::uvec4 ContentViewItem::prepareRectColor(const EntryInfo& info)
{
    if(info.bFolder)
    {
        return {144,156,155,100};
    }

    return {245,146,135,100};
}


ContentViewItem::ContentViewItem(const EntryInfo& info,WidgetAsset* widget,ContentView* view)
    : m_entryInfo(info), m_widget(widget), m_view(view)
{

}

void ContentViewItem::draw(const ViewItemDetail& detail)
{
    IconInfo* icon = prepareIcon(m_entryInfo, m_widget->getUISystem()->getIconLibrary());

    float thumbnailSize = (float)detail.thumbnailSize;

    static const float shadow_thickness = 2.0f;
    ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_BorderShadow];
    ImGui::GetWindowDrawList()->AddRectFilled(
        detail.rectButton.Min,
        ImVec2(detail.rectLabel.Max.x + shadow_thickness, detail.rectLabel.Max.y + shadow_thickness),
        IM_COL32(color.x * 255, color.y * 255, color.z * 255, color.w * 255));
    ImGui::PushID(m_entryInfo.filenameString.c_str());
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0,0,0,0));
    {
        // back color.
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f,1.0f,1.0f,0.05f));

        // thumbnail
        if(ImGui::Button("##dummy",
            ImVec2((float)detail.thumbnailSize,(float)detail.thumbnailSize)))
        {
            const auto now = std::chrono::high_resolution_clock::now();
            m_thumbnailInfo.timeSinceLastClick = now - m_thumbnailInfo.lastClickTime;
            m_thumbnailInfo.lastClickTime = now;

            const bool bSingleClick = m_thumbnailInfo.timeSinceLastClick.count() > 500;

            // event handle.

            // double click
            if(!bSingleClick)
            {
                // double click folder.
                if(m_entryInfo.bFolder)
                {

                }
                // double click file.
                else
                {

                }
            }
            // single click
            else if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {

            }
        }

        if(ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
        {
            m_bHoveringItem = true;
        }

        // Right click menu.
        if(ImGui::IsItemClicked(1) && m_view->m_bHoveringWindow && m_bHoveringItem && !m_entryInfo.bFolder)
        {
            ImGui::OpenPopup("##ItemRightClickContextMenu");
        }
        if(ImGui::BeginPopup("##ItemRightClickContextMenu"))
        {
            ImGui::Separator();
            if (ImGui::MenuItem("Rename"))
            {
                // todo: rename.
            }
            ImGui::EndPopup();
        }

        // draw image.
        {
            ImVec2 imageSizeMax = ImVec2(
                detail.rectButton.Max.x - detail.rectButton.Min.x - ImGui::GetStyle().FramePadding.x * 2.0f,
                detail.rectButton.Max.y - detail.rectButton.Min.y - ImGui::GetStyle().FramePadding.y * 2.0f);

            ImVec2 imageSize = ImVec2(
                static_cast<float>(icon->getIcon()->getExtent().width),
                static_cast<float>(icon->getIcon()->getExtent().height));
            ImVec2 imageSizeDelta = ImVec2(0.0f,0.0f);
            {
                if(imageSize.x != imageSizeMax.x)
                {
                    float scale = imageSizeMax.x/imageSize.x;
                    imageSize.x = imageSizeMax.x;
                    imageSize.y = imageSize.y*scale;
                }
                if(imageSize.y != imageSizeMax.y)
                {
                    float scale = imageSizeMax.y/imageSize.y;
                    imageSize.x = imageSize.x*scale;
                    imageSize.y = imageSizeMax.y;
                }
                imageSizeDelta.x = imageSizeMax.x-imageSize.x;
                imageSizeDelta.y = imageSizeMax.y-imageSize.y;
            }

            ImGui::SetCursorScreenPos(ImVec2(
                detail.rectButton.Min.x + ImGui::GetStyle().FramePadding.x + imageSizeDelta.x * 0.5f,
                detail.rectButton.Min.y + ImGui::GetStyle().FramePadding.y + imageSizeDelta.y * 0.5f));

            ImGui::Image((ImTextureID)icon->getId(),imageSize,ImVec2(0,1),ImVec2(1,0));

            auto color = prepareRectColor(m_entryInfo);
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(color.x, color.y, color.z, color.w));
        }

        ImGui::PopStyleColor(2);
        ImGui::PopID();
    }

    // draw label.
    {
        const uint32_t maxChar = 45;
        std::string displayStr = m_entryInfo.filenameString;
        if(m_entryInfo.filenameString.size() > maxChar)
        {
            displayStr = m_entryInfo.filenameString.substr(0, maxChar) + "...";
        }

        const char*  labelText = displayStr.c_str();
        const ImVec2 labelSize = ImGui::CalcTextSize(labelText,nullptr,true);
        const float textOffset = 3.0f;

        ImGui::SetCursorScreenPos(ImVec2(
            detail.rectLabel.Min.x,
            detail.rectLabel.Min.y + textOffset
        ));

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + thumbnailSize);
        
        ImGui::GetWindowDrawList()->AddRectFilled(detail.rectLabel.Min, detail.rectLabel.Max,IM_COL32(51,51,51,190));

        ImGui::Text(labelText);
        ImGui::PopTextWrapPos();
    }
}

bool ContentViewItem::shouldDraw() const
{
    if(m_entryInfo.bFolder) return true;
    if(isMetaFile(m_entryInfo.filePath)) return true;

    return false;
}
