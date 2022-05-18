#pragma once
#include <optional>
#include <string>

namespace flower
{
	struct LauncherInfo
	{
		std::optional<uint32_t>    width;
		std::optional<uint32_t>    height;
		std::optional<std::string> name; // tile name
		bool                       resizeable = true;
	};
}