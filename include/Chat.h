#pragma once

#include <iostream>
#include <vector>
#include "imgui.h"

class Chat
{
	char                  InputBuf[256]{};
	std::vector<char*>       Items;
	std::vector<const char*> Commands;
	bool                  AutoScroll;
	bool                  ScrollToBottom;

	void    ExecCommand(const char* command_line);
public:
	Chat();
	~Chat();

	void    ClearLog();

	void    AddLog(const char* fmt, ...);

	void    Draw();

	char* buffer;
	static Chat* i;
};
