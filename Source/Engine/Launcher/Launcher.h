#pragma once
#include <glfw/glfw3.h>
#include "LauncherInfo.h"

namespace flower
{
	class Launcher
	{
	public:
		static bool preInit(const LauncherInfo& info = {});
		static bool init();
		static void guardedMain();
		static void release();
	};
}