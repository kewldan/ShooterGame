#pragma once

#include <iostream>
#include <vector>
#include "imgui.h"

class Chat
{
	char* inputBuffer;
	std::vector<char*> Items;
	bool AutoScroll;
	bool ScrollToBottom;
	void ExecCommand(const char* command_line);
public:
    bool visible = true;
	Chat();
	~Chat();

	void ClearLog();
	void print(const char* fmt, ...);
	void Draw();

    static void init();

	char* message;
	static Chat* i;
};
