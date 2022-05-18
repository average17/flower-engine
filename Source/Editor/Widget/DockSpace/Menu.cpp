#include "Menu.h"

#include <Engine/Core/Core.h>
#include <Engine/Core/WindowData.h>
#include <ImGui/ImGui.h>
#include <ImGui/IconsFontAwesome5.h>

#include "../Viewport/WidgetViewport.h"
#include "../Console/WidgetConsole.h"
#include "../Hierarchy/WidgetHierarchy.h"
#include "../Inspector/WidgetInspector.h"
#include "../Asset/WigetAsset.h"
#include "../WidgetCommon.h"

using namespace flower;

const std::string FILE_ICON = ICON_FA_FILE;
const std::string WINDOW_ICON = ICON_FA_TOOLBOX;
const std::string CHECK_ICON = ICON_FA_CHECK;
const std::string CLOSE_ICON = "    ";

void Menu::buildFiles()
{
    if (ImGui::BeginMenu((" " + FILE_ICON+" File").c_str()))
    {
        ImGui::Separator();

        this->buildCloseCommand();

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu((" " + WINDOW_ICON + " Window").c_str()))
    {
        ImGui::Separator();

        this->buildWindowCommand();

        ImGui::EndMenu();
    }
}

void Menu::buildCloseCommand()
{
    
    if(ImGui::MenuItem("Close", NULL, false, GLFWWindowData::get()->getShouldRun() != nullptr))
    {
        *GLFWWindowData::get()->getShouldRun() = false;
    }
    
}

void Menu::buildWindowCommand()
{
    std::string iconString = WidgetReferences::viewport->getVisible() ? CHECK_ICON : CLOSE_ICON;
    if(ImGui::MenuItem((iconString + "  Viewport").c_str(),NULL,false,GLFWWindowData::get()->getShouldRun()!=nullptr))
    {
        WidgetReferences::viewport->setVisible(!WidgetReferences::viewport->getVisible());
    }

    iconString = WidgetReferences::asset->getVisible() ? CHECK_ICON : CLOSE_ICON;
    if(ImGui::MenuItem((iconString + "  Asset").c_str(),NULL,false,GLFWWindowData::get()->getShouldRun()!=nullptr))
    {
        WidgetReferences::asset->setVisible(!WidgetReferences::asset->getVisible());
    }

    iconString = WidgetReferences::hierarhcy->getVisible() ? CHECK_ICON : CLOSE_ICON;
    if(ImGui::MenuItem((iconString + "  Hierarchy").c_str(),NULL,false,GLFWWindowData::get()->getShouldRun()!=nullptr))
    {
        WidgetReferences::hierarhcy->setVisible(!WidgetReferences::hierarhcy->getVisible());
    }

    iconString = WidgetReferences::console->getVisible() ? CHECK_ICON : CLOSE_ICON;
    if(ImGui::MenuItem((iconString + "  Console").c_str(),NULL,false,GLFWWindowData::get()->getShouldRun()!=nullptr))
    {
        WidgetReferences::console->setVisible(!WidgetReferences::console->getVisible());
    }

    iconString = WidgetReferences::inspector->getVisible() ? CHECK_ICON : CLOSE_ICON;
    if(ImGui::MenuItem((iconString + "  Inspector").c_str(),NULL,false,GLFWWindowData::get()->getShouldRun()!=nullptr))
    {
        WidgetReferences::inspector->setVisible(!WidgetReferences::inspector->getVisible());
    }
}

void Menu::build()
{
    if (ImGui::BeginMenuBar())
    {
        this->buildFiles();
        ImGui::EndMenuBar();
    }
}