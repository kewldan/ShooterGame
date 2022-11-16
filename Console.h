#ifndef SHOOTERGAME_CONSOLE_H
#define SHOOTERGAME_CONSOLE_H

#include "imgui.h"

struct AppConsole
{
	char                  InputBuf[256];
	ImVector<char*>       Items;
	ImVector<const char*> Commands;
	bool                  AutoScroll;
	bool                  ScrollToBottom;

	AppConsole()
	{
		ClearLog();
		memset(InputBuf, 0, sizeof(InputBuf));

		Commands.push_back("?help");
		AutoScroll = true;
		ScrollToBottom = false;
		buffer = new char[256];
	}
	~AppConsole()
	{
		ClearLog();
	}

	// Portable helpers
	static int   Stricmp(const char* s1, const char* s2) { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
	static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
	static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
	static void  Strtrim(char* s) { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

	void    ClearLog()
	{
		for (int i = 0; i < Items.Size; i++)
			free(Items[i]);
		Items.clear();
	}

	void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
	{
		// FIXME-OPT
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		va_end(args);
		Items.push_back(Strdup(buf));
	}

	void    Draw()
	{
		ImGui::SetNextWindowPos(ImVec2(600, 20), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
		if (!ImGui::Begin("Console", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::End();
			return;
		}
		bool copy_to_clipboard = false;

		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
		if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar))
		{
			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::Selectable("Clear")) ClearLog();
				if (ImGui::Selectable("Copy")) {
					copy_to_clipboard = true;
				}
				ImGui::Checkbox("Auto scroll", &AutoScroll);
				ImGui::EndPopup();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
			if (copy_to_clipboard)
				ImGui::LogToClipboard();
			for (int i = 0; i < Items.Size; i++)
			{
				const char* item = Items[i];

				ImVec4 color;
				bool has_color = false;
				if (strstr(item, "[error]")) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
				else if (strstr(item, "[success]")) { color = ImVec4(0.2f, 1.f, 0.2f, 1.0f); has_color = true; }
				else if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
				if (has_color)
					ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::TextUnformatted(item);
				if (has_color)
					ImGui::PopStyleColor();
			}
			if (copy_to_clipboard)
				ImGui::LogFinish();

			if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
				ImGui::SetScrollHereY(1.0f);
			ScrollToBottom = false;

			ImGui::PopStyleVar();
		}
		ImGui::EndChild();
		ImGui::Separator();

		// Command-line
		bool reclaim_focus = false;
		if (ImGui::InputText("##InputCommand", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll))
		{
			char* s = InputBuf;
			Strtrim(s);
			if (s[0])
				ExecCommand(s);
			strcpy(s, "");
			reclaim_focus = true;
		}

		ImGui::SetItemDefaultFocus();
		if (reclaim_focus)
			ImGui::SetKeyboardFocusHere(-1);

		ImGui::End();
	}

	void    ExecCommand(const char* command_line)
	{
		if (command_line[0] == '?') {
			AddLog("# %s\n", command_line);
			if (Stricmp(command_line, "?help") == 0)
			{
				AddLog("Commands:");
				for (int i = 0; i < Commands.Size; i++)
					AddLog("- %s", Commands[i]);
			}
			else
			{
				AddLog("Unknown command: '%s'\n", command_line);
			}
		}
		else {
			strcpy(buffer, command_line);
		}

		ScrollToBottom = true;
	}

	public:
		char* buffer;
		static AppConsole* i;

		static void setInstance(AppConsole* console) {
			i = console;
		};
};

#endif