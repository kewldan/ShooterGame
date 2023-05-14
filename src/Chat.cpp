#include "Chat.h"


Chat::Chat() {
	ClearLog();
	memset(InputBuf, 0, sizeof(InputBuf));

	Commands.push_back("?help");
	AutoScroll = true;
	ScrollToBottom = false;
	buffer = new char[256];
	strcpy_s(buffer, 256, "");
}

Chat::~Chat() {
	ClearLog();
}

void Chat::ClearLog() {
	for (auto & Item : Items)
		free(Item);
	Items.clear();
}

void Chat::AddLog(const char* fmt, ...) {
	static char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
	buf[IM_ARRAYSIZE(buf) - 1] = 0;
	va_end(args);
	Items.push_back(strdup(buf));
}

void Chat::Draw() {
	ImGui::SetNextWindowPos(ImVec2(600, 20), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
	if (!ImGui::Begin("Console", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
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
		for (int i = 0; i < Items.size(); i++)
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

void Chat::ExecCommand(const char* command_line) {
	if (command_line[0] == '?') {
		AddLog("# %s\n", command_line);
		if (strcmp(command_line, "?help") == 0)
		{
			AddLog("Commands:");
			for (int i = 0; i < Commands.size(); i++)
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