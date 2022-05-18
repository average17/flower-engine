#pragma once
#include <string>
#include <filesystem>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <ImGui/ImGui.h>
#include <ImGui/ImGuiInternal.h>

class WidgetAsset;

namespace flower
{
	class IconLibrary;
	class IconInfo;
}

struct EntryInfo
{
	bool bFolder;

	// show file name.
	std::string filenameString;

	// note: on root it is relative path.
	// absolute path. use for assetsystem key.
	std::string filePath;

	const wchar_t* itemPath;

	std::filesystem::path path;
};

struct EntryInfoCompare
{
	bool operator()(const EntryInfo& a, const EntryInfo& b);
};

struct ViewItemDetail
{
	int32_t thumbnailSize = 100;
	ImRect rectButton;
	ImRect rectLabel;

	float labelHeight;
	float buttomOffset;
	float contentWidth;
	float contentHeight;

	void updateRect();
	void updateBasic();
};

class ContentView;