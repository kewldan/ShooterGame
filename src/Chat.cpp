#include "Chat.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

void Chat::ClearLog() {
	Items.clear();
}

void Chat::print(const char* fmt, ...) {
	char buf[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	buf[sizeof(buf) - 1] = 0;
	va_end(args);
	Items.emplace_back(buf);
}

void Chat::Draw() {
	ImGui::SetNextWindowPos(ImVec2(600, 20), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
	// Starts folded: opened it covers the crosshair.
	ImGui::SetNextWindowCollapsed(!startExpanded, ImGuiCond_Once);
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
		for (const auto& item : Items)
		{
			ImVec4 color;
			bool has_color = false;
			if (item.find("[error]") != std::string::npos) { color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true; }
			else if (item.find("[success]") != std::string::npos) { color = ImVec4(0.2f, 1.f, 0.2f, 1.0f); has_color = true; }
			else if (item.compare(0, 2, "# ") == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
			if (has_color)
				ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(item.c_str());
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
	if (ImGui::InputText("##InputCommand", inputBuffer, sizeof(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll))
	{
		if (inputBuffer[0])
			ExecCommand(inputBuffer);
		inputBuffer[0] = 0;
		reclaim_focus = true;
	}

	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1);

	ImGui::End();
}

void Chat::submit(const char* line) {
	if (line && line[0]) ExecCommand(line);
}

void Chat::ExecCommand(const char* command_line) {
	if (command_line[0] == '?') {
        print("# %s", command_line);
		if (onCommand && onCommand(command_line)) {
			// handled by the game
		}
		else if (strcmp(command_line, "?help") == 0)
		{
            print("Commands:");
            print("- ?help");
            print("- ?players: who is connected, with ping");
		}
		else
		{
            print("Unknown command: '%s'", command_line);
		}
	}
	else if (onMessage) {
		onMessage(command_line);
	}
	else {
		print("%s", command_line);
	}

	ScrollToBottom = true;
}

void Chat::init() {
    if (!Chat::i) {
        Chat::i = std::make_unique<Chat>();
    }
}

std::unique_ptr<Chat> Chat::i;
