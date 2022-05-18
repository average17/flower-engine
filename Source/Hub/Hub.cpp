#include "Hub.h"

#include <Engine/Launcher/Launcher.h>
#include <Engine/Core/Core.h>
#include <Engine/Engine.h>
#include <Engine/Renderer/Renderer.h>
#include <Engine/UI/UISystem.h>

#include "Widget/DockSpace.h"

using namespace flower;

std::vector<std::unique_ptr<Widget>> GWidgets { };

void Hub::init()
{
	LauncherInfo info { };
	info.width  = 900;
	info.height = 500;
	info.name   = "flower hub";
	info.resizeable = false;

	CHECK(Launcher::preInit(info));

	GEngine.registerRuntimeModule<UISystem>();
	GEngine.registerRuntimeModule<Renderer>();

	CHECK(Launcher::init());

	// register widgets here.
	GWidgets.push_back(std::make_unique<DockSpace>(&GEngine));


	// init all widgets.
	for(const auto& ptr : GWidgets)
	{
		ptr->init();
	}
}

void Hub::loop()
{
	Launcher::guardedMain();
}

void Hub::release()
{
	// release all widgets first.
	for(const auto& ptr : GWidgets)
	{
		ptr->release();
	}

	Launcher::release();
}
