#include "Console.h"
#include <Engine/Core/Core.h>
#include <Engine/Renderer/Renderer.h>
#include <ImGui/IconsFontAwesome5.h>
#include <string>

std::string FILTER_ICON = ICON_FA_FILTER;
std::string CLONE_ICON = ICON_FA_COPY;
std::string BROOM_ICON = ICON_FA_BROOM;

using namespace flower;

void Console::clearLog()
{
    m_logItems.clear();
    for (auto& i : m_logTypeCount)
    {
        i = 0;
    }
}

void Console::addLog(std::string info, ELogType type)
{
    m_logTypeCount[size_t(type)] ++;
    m_logItems.push_back({ info, type });

    if (static_cast<uint32_t>(m_logItems.size()) >= MAX_LOG_ITEMS_COUNT)
    {
        m_logItems.pop_front();
    }
}

void Console::init()
{
    auto& int32Array  = CVarSystem::get()->getInt32Array();
    auto& floatArray  = CVarSystem::get()->getFloatArray();
    auto& doubleArray = CVarSystem::get()->getDoubleArray();
    auto& stringArray = CVarSystem::get()->getStringArray();

    auto parseArray = [&](auto& cVarsArray)
    {
        auto typeBit = cVarsArray.lastCVar;
        while (typeBit > 0)
        {
            typeBit--;
            auto storageValue = cVarsArray.getCurrentStorage(typeBit);
            m_commands.push_back(storageValue->parameter->name);
        }
    };

    clearLog();
    memset(m_inputBuffer, 0, sizeof(m_inputBuffer));
    m_historyPos = -1;

    parseArray(int32Array);
    parseArray(floatArray);
    parseArray(doubleArray);
    parseArray(stringArray);

    m_bAutoScroll = true;
    m_bScrollToBottom = false;

    m_logCacheId = Logger::getInstance()->getLoggerCache()->pushCallback([&](std::string info, flower::ELogType type) 
    {
        this->addLog(info, type);
    });
}



void Console::release()
{
    Logger::getInstance()->getLoggerCache()->popCallback(m_logCacheId);

    clearLog();
    for (int i = 0; i < m_historyCommands.Size; i++)
    {
        free(m_historyCommands[i]);
    }
}

namespace 
{
    static int myStricmp(const char* s1, const char* s2)
    {
        int d;
        while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
        {
            s1++;
            s2++;
        }
        return d;
    }

    static int myStrnicmp(const char* s1, const char* s2, int n)
    {
        int d = 0;
        while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
        {
            s1++;
            s2++;
            n--;
        }
        return d;
    }

    static char* myStrdup(const char* s) 
    {
        IM_ASSERT(s); 
        size_t len = strlen(s) + 1; 
        void* buf = malloc(len); 
        IM_ASSERT(buf); 
        return (char*)memcpy(buf, (const void*)s, len); 
    }

    static void myStrtrim(char* s) 
    { 
        char* str_end = s + strlen(s); 
        while (str_end > s && str_end[-1] == ' ') 
            str_end--; 

        *str_end = 0; 
    }

    static int textEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        Console* console = (Console*)data->UserData;
        return console->textEditCallback(data);
    }
}

void Console::addLog(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);

    addLog(std::string(myStrdup(buf)), ELogType::Other);
}

void Console::execCommand(const char* command)
{
    addLog("# %s\n", command);

    // Insert into history. First find match and delete it so it can be pushed to the back.
    // This isn't trying to be smart or optimal.
    m_historyPos = -1;
    for (int i = m_historyCommands.Size - 1; i >= 0; i--)
    {
        if (myStricmp(m_historyCommands[i], command) == 0)
        {
            free(m_historyCommands[i]);
            m_historyCommands.erase(m_historyCommands.begin() + i);
            break;
        }
    }
        
    m_historyCommands.push_back(myStrdup(command));

    std::string cmd_info = command;
    std::regex ws_re("\\s+"); // whitespace

    std::vector<std::string> tokens(
        std::sregex_token_iterator(cmd_info.begin(), cmd_info.end(), ws_re, -1),
        std::sregex_token_iterator()
    );

    auto cVar = CVarSystem::get()->getCVar(tokens[0].c_str());
    if (cVar != nullptr)
    {
        if (tokens.size() == 1)
        {
            int32_t arrayIndex = cVar->arrayIndex;
            uint32_t flag = cVar->flag;
            std::string val;
            if (cVar->type == CVarType::Int32)
            {
                val = std::to_string(CVarSystem::get()->getInt32Array().getCurrent(arrayIndex));
            }
            else if (cVar->type == CVarType::Double)
            {
                val = std::to_string(CVarSystem::get()->getDoubleArray().getCurrent(arrayIndex));
            }
            else if (cVar->type == CVarType::Float)
            {
                val = std::to_string(CVarSystem::get()->getFloatArray().getCurrent(arrayIndex));
            }
            else if (cVar->type == CVarType::String)
            {
                val = CVarSystem::get()->getStringArray().getCurrent(arrayIndex);
            }

            {
                std::string outHelp = "Help: " + std::string(cVar->description);
                addLog(outHelp.c_str());
                addLog(("Current value: " + tokens[0] + " " + val).c_str());
            }
        }
        else if (tokens.size() == 2)
        {
            int32_t arrayIndex = cVar->arrayIndex;
            uint32_t flag = cVar->flag;

            if ((flag & InitOnce) || (flag & ReadOnly))
            {
                addLog("'%s' is a readonly value, can't change on console!\n", tokens[0].c_str());
            }
            else if (cVar->type == CVarType::Int32)
            {
                CVarSystem::get()->getInt32Array().setCurrent((int32_t)std::stoi(tokens[1]), cVar->arrayIndex);
            }
            else if (cVar->type == CVarType::Double)
            {
                CVarSystem::get()->getDoubleArray().setCurrent((double)std::stod(tokens[1]), cVar->arrayIndex);
            }
            else if (cVar->type == CVarType::Float)
            {
                CVarSystem::get()->getFloatArray().setCurrent((float)std::stof(tokens[1]), cVar->arrayIndex);
            }
            else if (cVar->type == CVarType::String)
            {
                CVarSystem::get()->getStringArray().setCurrent(tokens[1], cVar->arrayIndex);
            }

            if (flag & RenderStateRelative)
            {
                GRenderStateMonitor.broadcast();
            }
        }
        else
        {
            addLog("Error command parameters, all CVar only keep one parameter, but '%d' paramters input this time.\n", tokens.size() - 1);
        }
    }
    else
    {
        addLog("Unkonwn command: '%s'!\n", command);
    }

    // On command input, we scroll to bottom even if AutoScroll==false
    m_bScrollToBottom = true;
}

void Console::inputCallbackOnComplete(ImGuiInputTextCallbackData* data)
{
    // locate beginning of current word
    const char* word_end = data->Buf + data->CursorPos;
    const char* word_start = word_end;
    while (word_start > data->Buf)
    {
        const char c = word_start[-1];
        if (c == ' ' || c == '\t' || c == ',' || c == ';')
            break;
        word_start--;
    }

    // build a list of candidates
    ImVector<const char*> candidates;
    for (int i = 0; i < m_commands.Size; i++)
    {
        if (myStrnicmp(m_commands[i], word_start, (int)(word_end - word_start)) == 0)
        {
            candidates.push_back(m_commands[i]);
        }
        else
        {
            std::string lowerCommand = m_commands[i];
            std::transform(lowerCommand.begin(), lowerCommand.end(), lowerCommand.begin(), ::tolower);

            if (myStrnicmp(lowerCommand.c_str(), word_start, (int)(word_end - word_start)) == 0)
            {
                candidates.push_back(m_commands[i]);
            }
        }
    }

    if (candidates.Size == 0)
    {
        // No match
    }
    else if (candidates.Size == 1)
    {
        // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
        data->InsertChars(data->CursorPos, candidates[0]);
        data->InsertChars(data->CursorPos, " ");
    }
    else
    {
        // Multiple matches. Complete as much as we can..
        // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
        int match_len = (int)(word_end - word_start);
        for (;;)
        {
            int c = 0;
            bool all_candidates_matches = true;
            for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                if (i == 0)
                    c = toupper(candidates[i][match_len]);
                else if (c == 0 || c != toupper(candidates[i][match_len]))
                    all_candidates_matches = false;
            if (!all_candidates_matches)
                break;
            match_len++;
        }

        if (match_len > 0)
        {
            data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
            data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
        }
    }
}

void Console::inputCallbackOnEdit(ImGuiInputTextCallbackData* data)
{
    m_activeCommands.clear();

    // locate beginning of current word
    const char* word_end = data->Buf + data->CursorPos;
    const char* word_start = word_end;
    while (word_start > data->Buf)
    {
        const char c = word_start[-1];
        if (c == ' ' || c == '\t' || c == ',' || c == ';')
            break;
        word_start--;
    }

    // build a list of candidates
    ImVector<const char*> candidates;
    for (int i = 0; i < m_commands.Size; i++)
    {
        int size = (int)(word_end - word_start);

        if (size > 0 && myStrnicmp(m_commands[i], word_start, size) == 0)
        {
            candidates.push_back(m_commands[i]);
        }
        else
        {
            std::string lowerCommand = m_commands[i];
            std::transform(lowerCommand.begin(), lowerCommand.end(), lowerCommand.begin(), ::tolower);

            if (size > 0 && myStrnicmp(lowerCommand.c_str(), word_start, size) == 0)
            {
                candidates.push_back(m_commands[i]);
            }
        }
    }

    if (candidates.Size >= 1)
    {
        for (int i = 0; i < candidates.Size; i++)
        {
            m_activeCommands.push_back(candidates[i]);
        }
    }
}

void Console::inputCallbackOnHistory(ImGuiInputTextCallbackData* data)
{
    const int prev_history_pos = m_historyPos;
    if (data->EventKey == ImGuiKey_UpArrow)
    {
        if (m_historyPos == -1)
            m_historyPos = m_historyCommands.Size - 1;
        else if (m_historyPos > 0)
            m_historyPos--;
    }
    else if (data->EventKey == ImGuiKey_DownArrow)
    {
        if (m_historyPos != -1)
            if (++m_historyPos >= m_historyCommands.Size)
                m_historyPos = -1;
    }

    // A better implementation would preserve the data on the current input line along with cursor position.
    if (prev_history_pos != m_historyPos)
    {
        const char* history_str = (m_historyPos >= 0) ? m_historyCommands[m_historyPos] : "";
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, history_str);
    }
}

int Console::textEditCallback(ImGuiInputTextCallbackData* data)
{
    // do event callback.
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        inputCallbackOnComplete(data);
        break;
    }
    case ImGuiInputTextFlags_CallbackEdit:
    {
        inputCallbackOnEdit(data);
        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        inputCallbackOnHistory(data);
        break;
    }
    }
    return 0;
}

void Console::draw()
{
    // Log type visibility toggle.
    {
        const auto button_log_type_visibility_toggle = [this](const uint32_t icon, ELogType index, std::string Name)
        {
            bool& visibility = m_logVisible[(size_t)index];

            ImGui::Checkbox((Name + ":").c_str(), &visibility);
            ImGui::SameLine();
            if (m_logTypeCount[(size_t)index] <= 99)
            {
                ImGui::Text("%d", m_logTypeCount[(size_t)index]);
            }
            else
            {
                ImGui::Text("99+");
            }
        };

        // Log category visibility buttons
        button_log_type_visibility_toggle(0, ELogType::Trace, "Message");
        ImGui::SameLine();
        button_log_type_visibility_toggle(0, ELogType::Info, "Info");
        ImGui::SameLine();
        button_log_type_visibility_toggle(0, ELogType::Warn, "Warn");
        ImGui::SameLine();
        button_log_type_visibility_toggle(0, ELogType::Error, "Error");
        ImGui::SameLine();
        button_log_type_visibility_toggle(0, ELogType::Other, "Other");
    }

    ImGui::Separator();

    // menu commands.
    bool copy_to_clipboard;
    {
        m_filter.Draw(FILTER_ICON.c_str(),180);
        ImGui::SameLine();
        
        if (ImGui::Button((" " +BROOM_ICON+" Clear").c_str()))
        {
            clearLog();
        }
        ImGui::SameLine();
        copy_to_clipboard = ImGui::Button((" " +CLONE_ICON+" Copy").c_str());
    }

    ImGui::Separator();

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
    if (ImGui::BeginPopupContextWindow())
    {
        if (ImGui::Selectable("Clear"))
        {
            clearLog();
        }
        ImGui::EndPopup();
    }

    // Tighten spacing
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
    if (copy_to_clipboard)
    {
        ImGui::LogToClipboard();
    }

    for (int i = 0; i < m_logItems.size(); i++)
    {
        const char* item = m_logItems[i].first.c_str();
        if (!m_filter.PassFilter(item))
            continue;

        ImVec4 color;
        bool has_color = false;
        if (m_logItems[i].second == ELogType::Error)
        {
            if (!m_logVisible[size_t(ELogType::Error)])
                continue;

            color = ImVec4(1.0f, 0.08f, 0.08f, 1.0f);
            has_color = true;
        }
        else if (m_logItems[i].second == ELogType::Warn)
        {
            if (!m_logVisible[size_t(ELogType::Warn)])
                continue;

            color = ImVec4(1.0f, 1.0f, 0.1f, 1.0f);
            has_color = true;
        }
        else if (m_logItems[i].second == ELogType::Trace)
        {
            if (!m_logVisible[size_t(ELogType::Trace)])
                continue;

            color = ImVec4(0.1f, 1.0f, 0.1f, 1.0f);
            has_color = true;
        }
        else if (m_logItems[i].second == ELogType::Info)
        {
            if (!m_logVisible[size_t(ELogType::Info)])
                continue;
        }
        else if (strncmp(item, "# ", 2) == 0)
        {
            if (!m_logVisible[size_t(ELogType::Other)])
                continue;

            color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
            has_color = true;
        }
        else if (strncmp(item, "Help: ", 5) == 0)
        {
            if (!m_logVisible[size_t(ELogType::Other)])
                continue;

            color = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
            has_color = true;
        }
        else
        {
            if (!m_logVisible[size_t(ELogType::Other)])
                continue;
        }

        if (has_color)
            ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(item);
        if (has_color)
            ImGui::PopStyleColor();
    }

    if (copy_to_clipboard)
    {
        ImGui::LogFinish();
    }

    if (m_bScrollToBottom || (m_bAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
    {
        ImGui::SetScrollHereY(1.0f);
    }

    m_bScrollToBottom = false;

    ImGui::PopStyleVar();
    ImGui::EndChild();

    ImGui::Separator();

    // Command-line
    bool reclaim_focus = false;
    ImGuiInputTextFlags input_text_flags =
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_CallbackEdit |
        ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory;

    auto TipPos = ImGui::GetWindowPos();

    TipPos.x += ImGui::GetStyle().WindowPadding.x;
    const float WindowHeight = ImGui::GetWindowHeight();
    const auto ItemSize = ImGui::GetTextLineHeightWithSpacing();
    TipPos.y = TipPos.y + WindowHeight - (m_activeCommands.size() + 2.5f) * ItemSize;
    if (ImGui::InputText(" Cvar Input", m_inputBuffer, IM_ARRAYSIZE(m_inputBuffer), input_text_flags, &textEditCallbackStub, (void*)this))
    {
        char* s = m_inputBuffer;
        myStrtrim(s);
        if (s[0])
        {
            execCommand(s);
            m_activeCommands.clear();
        }

        strcpy(s, "");
        reclaim_focus = true;
    }

    if (m_activeCommands.size() > 0 && ImGui::IsWindowFocused())
    {
        ImGui::BeginTooltipWithPos(TipPos);
        for (auto& info : m_activeCommands)
        {
            ImGui::TextUnformatted(info);
        }
        ImGui::EndTooltipWithPos();
    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if (reclaim_focus)
        ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
}