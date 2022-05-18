#pragma once
#include <ImGui/ImGui.h>
#include <malloc.h>
#include <ctype.h>
#include <stdio.h>
#include <mutex>
#include <deque>
#include <regex>
#include "Engine/Core/Log.h"

class Console
{
private:
	int m_logCacheId;

	// command input buffer.
	char m_inputBuffer[256];

	// precache commands on cvar system.
	ImVector<const char*> m_commands;
	ImVector<char*> m_historyCommands;
	std::vector<const char*> m_activeCommands;
	
	// -1 is new line, [0, m_historyCommands.size() - 1] is browsing history.
	int32_t m_historyPos;

	// filter for log items.
	ImGuiTextFilter m_filter;

	// whether auto scroll if log items full.
	bool m_bAutoScroll = true;
	bool m_bScrollToBottom;

	// log items deque.
	std::deque<std::pair<std::string, flower::ELogType>> m_logItems;
	inline static const uint32_t MAX_LOG_ITEMS_COUNT = 200;

	bool m_logVisible[(size_t)flower::ELogType::Max] = {
		true, // Trace
		true, // Info
		true, // Warn
		true, // Error
		true  // Other
	};
	uint32_t m_logTypeCount[(size_t)flower::ELogType::Max] = {
		0, // Trace
		0, // Info
		0, // Warn
		0, // Error
		0  // Other
	};

private:
	void clearLog();
	void addLog(std::string info, flower::ELogType type);
	void addLog(const char* fmt, ...);

	void execCommand(const char* command);

	void inputCallbackOnComplete(ImGuiInputTextCallbackData* data);
	void inputCallbackOnEdit(ImGuiInputTextCallbackData* data);
	void inputCallbackOnHistory(ImGuiInputTextCallbackData* data);

public:
	Console() = default;

	void init();
	void draw();
	void release();

	int32_t textEditCallback(ImGuiInputTextCallbackData* data);
};