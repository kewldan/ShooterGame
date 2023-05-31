#pragma once

#include <iostream>
#include <vector>
#include "imgui.h"

class Chat
{
	char* inputBuffer;
	std::vector<char*> Items;
	std::vector<const char*> Commands;
	bool AutoScroll;
	bool ScrollToBottom;
	void ExecCommand(const char* command_line);
public:
	Chat();
	~Chat();

	void ClearLog();
	void print(const char* fmt, ...);
	void Draw();

	char* message;
	static Chat* i;
};
