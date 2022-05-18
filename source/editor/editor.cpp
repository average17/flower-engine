#include "Editor.h"

#include <Engine/Launcher/Launcher.h>
#include <Engine/Core/Core.h>
#include <Engine/Engine.h>
#include <Engine/Renderer/DeferredRenderer/DefferedRenderer.h>
#include <Engine/UI/UISystem.h>
#include <Engine/Scene/SceneManager.h>
#include <Engine/Asset/AssetSystem.h>

#include "Widget/DockSpace/DockSpace.h"
#include "Widget/Asset/WigetAsset.h"
#include "Widget/Viewport/WidgetViewport.h"
#include "Widget/Hierarchy/WidgetHierarchy.h"
#include "Widget/Console/WidgetConsole.h"
#include "Widget/Downbar/Downbar.h"
#include "Widget/Inspector/WidgetInspector.h"
#include "Widget/WidgetCommon.h"

using namespace flower;

std::vector<std::unique_ptr<Widget>> GWidgets { };

void Editor::init()
{
	CHECK(Launcher::preInit());

	// prepare the modules we need.
	// asset tick -> ui tick -> scene tick -> renderer tick.
	GEngine.registerRuntimeModule<AssetSystem>();
	GEngine.registerRuntimeModule<UISystem>();
	GEngine.registerRuntimeModule<SceneManager>();
	GEngine.registerRuntimeModule<DefferedRenderer>();
	

	CHECK(Launcher::init());

	// register widgets here.
	GWidgets.push_back(std::make_unique<DockSpace>(&GEngine));
	GWidgets.push_back(std::make_unique<WidgetDownbar>(&GEngine));

	// console first.
	auto widgetConsole = std::make_unique<WidgetConsole>(&GEngine);
	WidgetReferences::console = widgetConsole.get();
	GWidgets.push_back(std::move(widgetConsole));

	auto assetWidget = std::make_unique<WidgetAsset>(&GEngine);
	WidgetReferences::asset = assetWidget.get();
	GWidgets.push_back(std::move(assetWidget));

	auto widgetViewport = std::make_unique<WidgetViewport>(&GEngine);
	WidgetReferences::viewport = widgetViewport.get();
	GWidgets.push_back(std::move(widgetViewport));

	auto widgetHierarhcy = std::make_unique<WidgetHierarhcy>(&GEngine);
	WidgetReferences::hierarhcy = widgetHierarhcy.get();
	GWidgets.push_back(std::move(widgetHierarhcy));

	auto widgetInspector = std::make_unique<WidgetInspector>(&GEngine);
	WidgetReferences::inspector = widgetInspector.get();
	GWidgets.push_back(std::move(widgetInspector));
	
	// init all widgets.
	for(const auto& ptr : GWidgets)
	{
		ptr->init();
	}
}

void Editor::loop()
{
	Launcher::guardedMain();
}

void Editor::release()
{
	// release all widgets first.
	for(const auto& ptr : GWidgets)
	{
		ptr->release();
	}

	Launcher::release();
}
