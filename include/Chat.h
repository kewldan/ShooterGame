#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "imgui.h"

class Chat
{
	char inputBuffer[256]{};
	std::vector<std::string> Items;
	bool AutoScroll = true;
	bool ScrollToBottom = false;
	void ExecCommand(const char* command_line);
public:
    bool visible = true;
    // The window starts collapsed (it covers the crosshair) unless this is set before the first Draw().
    bool startExpanded = false;
	Chat() = default;

	void ClearLog();
	void print(const char* fmt, ...);
	// As if `line` had been typed into the console (a command or a chat message).
	void submit(const char* line);
	void Draw();

    static void init();

	// Called with every non-command line typed into the console (a chat message); without a handler
	// the line is just echoed.
	std::function<void(const std::string &)> onMessage;
	// Gets a chance at every "?command" before the built-in ones; returns true when it handled it.
	std::function<bool(const std::string &)> onCommand;
	static std::unique_ptr<Chat> i;
};
