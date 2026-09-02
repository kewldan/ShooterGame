#pragma once

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
	Chat() = default;

	void ClearLog();
	void print(const char* fmt, ...);
	void Draw();

    static void init();

	// Last non-command line typed into the console (was meant to be sent over the network).
	char message[256]{};
	static std::unique_ptr<Chat> i;
};
