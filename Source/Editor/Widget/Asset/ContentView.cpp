#include "ContentView.h"
#include "WigetAsset.h"

#include <Engine/Core/Core.h>
#include <Engine/UI/Icon.h>
#include <Engine/UI/UISystem.h>
#include <Engine/RHI/RHI.h>
#include <crcpp/crc.h>
#include "../../Style/Icon.h"

#include <ImGui/IconsFontAwesome5.h>

using namespace flower;

void ContentView::viewContentDraw()
{
    bool bHoveringWindow = false;
    bool bNewline = true;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(0,0,0,50));

    m_itemDetail.updateBasic();

    float penXMin = 0.0f;
    float penX    = 0.0f;
    uint32_t displayItemCount = 0;

    if(ImGui::BeginChild("FlowerContentViewRegion",ImVec2(m_itemDetail.contentWidth,m_itemDetail.contentHeight),true))
    {
        bHoveringWindow = ImGui::IsWindowHovered(
            ImGuiHoveredFlags_AllowWhenBlockedByPopup|
            ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) ?
            true : bHoveringWindow;

        float offset = ImGui::GetStyle().ItemSpacing.x;
        penXMin = ImGui::GetCursorPosX() + offset;
        ImGui::SetCursorPosX(penXMin);

        // from -1 and we plus 1 when loop start.
        for(int32_t loopIndex = 0; loopIndex < m_contentItems.size(); loopIndex ++)
        {
            bool bShouldDraw = m_contentItems[loopIndex].shouldDraw();

            if(!bShouldDraw)
            {
                continue;
            }

            displayItemCount ++;
            if(bNewline)
            {
                ImGui::BeginGroup();
                bNewline = false;
            }
            ImGui::BeginGroup();
            {
                m_itemDetail.updateRect();
                m_contentItems[loopIndex].draw(m_itemDetail);
                ImGui::EndGroup();
            }

            // compute new line.
            penX += m_itemDetail.thumbnailSize + ImGui::GetStyle().ItemSpacing.x;
            if(penX >= m_itemDetail.contentWidth - m_itemDetail.thumbnailSize)
            {
                ImGui::EndGroup();
                penX = penXMin;
                ImGui::SetCursorPosX(penX);
                bNewline = true;
            }
            else
            {
                ImGui::SameLine();
            }
        }

        if(!bNewline)
        {
            ImGui::EndGroup();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    const bool bShowItemCounts = true;
    if(bShowItemCounts)
    {
        const float offsetBottom = ImGui::GetTextLineHeight() * 1.2f;
        ImGui::SetCursorPosY(ImGui::GetWindowSize().y - offsetBottom);
        ImGui::Text("%d Items", displayItemCount);
    }
}

void ContentView::buildContentTree()
{
    m_contentTreeRoot.reset();
    
    auto entry = std::filesystem::directory_entry(m_projectDirectory);

    std::filesystem::path projPathFullPath = std::filesystem::absolute(m_projectDirectory);
    EntryInfo cacheInfo{};

    cacheInfo.bFolder = entry.is_directory();
    cacheInfo.path = entry.path().filename();
    cacheInfo.filePath = entry.path().string();

    auto relativePath = std::filesystem::relative(entry.path(), m_projectDirectory);
    cacheInfo.filenameString = relativePath.filename().string();
    cacheInfo.itemPath = relativePath.c_str();

    m_contentTreeRoot = std::make_shared<ContentTreeItem>(cacheInfo, m_widget, this, nullptr);

    buildContentTreeRecursive(m_contentTreeRoot);
}

void ContentView::buildContentTreeRecursive(std::shared_ptr<ContentTreeItem> parent)
{
    std::vector<EntryInfo> scanEntryInfos { };

    // we use absolute path.
    std::filesystem::path iterPathFullPath = std::filesystem::absolute(parent->m_entryInfo.filePath);

    for (auto& directoryEntry : std::filesystem::directory_iterator(iterPathFullPath))
    {
        EntryInfo cacheInfo{};

        cacheInfo.bFolder = directoryEntry.is_directory();
        cacheInfo.path = directoryEntry.path().filename();
        cacheInfo.filePath = directoryEntry.path().string();

        auto relativePath = std::filesystem::relative(directoryEntry.path(), m_projectDirectory);

        cacheInfo.filenameString = relativePath.filename().string();
        cacheInfo.itemPath = relativePath.c_str();

        scanEntryInfos.push_back(cacheInfo);
    }

    std::sort(scanEntryInfos.begin(), scanEntryInfos.end(), m_entryInfoCompare);

    for(const auto& entryInfo : scanEntryInfos)
    {
        auto child = std::make_shared<ContentTreeItem>(entryInfo, m_widget, this, parent);
        if(parent != nullptr)
        {
            parent->addChild(child);
        }

        if(entryInfo.bFolder)
        {
            buildContentTreeRecursive(child);
        }
    }
}

void ContentView::drawTreeRecursive(std::shared_ptr<ContentTreeItem> item)
{
    const bool bTreeNode = item->m_children.size() > 0;

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanFullWidth;
    node_flags |= bTreeNode ? ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_Leaf; 

    std::string iconId;
    if(item->m_entryInfo.bFolder)
    {
        iconId = ICON_FA_FOLDER;
    }
    else
    {
        iconId = ICON_FA_FILE;
    }

    std::string name = iconId + "  " + item->m_entryInfo.filenameString;

    bool bNodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(
        static_cast<intptr_t>(CRC::Calculate(&item->m_entryInfo, sizeof(EntryInfo), CRC::CRC_32()))), 
        node_flags, 
        name.c_str());

    if(bNodeOpen)
    {
        if (bTreeNode)
        {
            for (const auto& child : item->m_children)
            {
                drawTreeRecursive(child);
            }
        }

        ImGui::TreePop();
    }
}


void ContentView::init(std::string projectDir, WidgetAsset* widget)
{
    m_widget = widget;
    if(projectDir != m_projectDirectory && std::filesystem::exists(projectDir))
    {
        m_projectDirectory = projectDir;
        m_workingDirectory = projectDir;

        m_contentItems.clear();
    }
}

void ContentView::drawTree()
{
    if(m_contentTreeRoot && m_contentTreeRoot->m_children.size() > 0)
    {
        for(auto& child : m_contentTreeRoot->m_children)
        {
            CHECK(child);
            drawTreeRecursive(child);
        }
    }
}

void ContentView::drawContent()
{
    // sync content view first.
    if(m_bNeedUpdated)
    {
        m_bNeedUpdated = false;

        // build view tree.
        m_contentTreeRoot.reset();
        {
            buildContentTree();
        }

        // update scanView for working path.
        m_contentItems.clear();
        {
            std::vector<EntryInfo> scanEntryInfos { };
            if(!std::filesystem::exists(m_workingDirectory))
            {
                LOG_WARN("Working directory: {} is no exist, do you forget update it?", m_workingDirectory);
                m_workingDirectory = m_projectDirectory;
                LOG_TRACE("Working directory switch to {}.", m_workingDirectory);
            }

            std::filesystem::path workingFullPath = std::filesystem::absolute(m_workingDirectory);

            for (auto& directoryEntry : std::filesystem::directory_iterator(workingFullPath))
            {
                EntryInfo cacheInfo{};

                cacheInfo.bFolder = directoryEntry.is_directory();
                cacheInfo.path    = directoryEntry.path().filename();
                cacheInfo.filePath = directoryEntry.path().string();

                auto relativePath = std::filesystem::relative(directoryEntry.path(), m_projectDirectory);

                cacheInfo.filenameString = relativePath.filename().string();
                cacheInfo.itemPath = relativePath.c_str();

                scanEntryInfos.push_back(cacheInfo);
            }

            std::sort(scanEntryInfos.begin(), scanEntryInfos.end(), m_entryInfoCompare);

            for(const auto& entryInfo : scanEntryInfos)
            {
                m_contentItems.push_back({ entryInfo, m_widget, this });
            }
        }
    }

    viewContentDraw();
}

void ContentView::setDirty()
{
    m_bNeedUpdated = true;
}