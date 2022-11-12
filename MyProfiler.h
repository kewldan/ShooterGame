#ifndef SHOOTERGAME_PROFILER_H
#define SHOOTERGAME_PROFILER_H

#include <map>
#include <string>
#include <chrono>

struct ProfilerBlock {
	unsigned long allTime;
	unsigned int iterations;
};

class MyProfiler {
	const char* lastBlockName;
	unsigned long lastBlockStart;
public:
	std::map<const char*, ProfilerBlock> blocks;

	MyProfiler();

	void startBlock(const char* name);

	void endBlock();
};


#endif //SHOOTERGAME_PROFILER_H
