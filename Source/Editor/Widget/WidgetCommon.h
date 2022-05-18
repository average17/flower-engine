#pragma once
#include <string>
#include <Engine/Core/Core.h>

class WidgetAsset;
class WidgetViewport;
class WidgetRenderGraph;
class WidgetHierarhcy;
class WidgetConsole;
class WidgetInspector;

struct WidgetReferences
{
	static WidgetAsset* asset;
	static WidgetViewport* viewport;
	static WidgetRenderGraph* renderGraph;
	static WidgetHierarhcy* hierarhcy;
	static WidgetConsole* console;
	static WidgetInspector* inspector;
};

class WidgetCommon
{
public:
	static void drawVector3(const std::string& label, glm::vec3& values, const glm::vec3& resetValue);
};