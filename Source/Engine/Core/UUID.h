#pragma once
#include <string>

namespace flower
{
	using UUID = std::string;
	constexpr auto UNVALID_UUID = "";

	extern std::string getUUID();
}