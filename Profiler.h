#ifndef SHOOTERGAME_PROFILER_H
#define SHOOTERGAME_PROFILER_H

#include "windows.h"
#include "psapi.h"

#include <map>
#include <string>
#include <chrono>

struct ProfilerBlock {
    unsigned long allTime;
    unsigned int iterations;
};

struct MemoryInfo {
    unsigned long virtualMemoryTotal;
    unsigned long virtualMemoryUsed;
    unsigned long physicalMemoryTotal;
    unsigned long physicalMemoryUsed;
};

class Profiler {
    std::string lastBlockName;
    unsigned long lastBlockStart;
public:
    std::map<std::string, ProfilerBlock> blocks;

    Profiler();

    void startBlock(std::string name);
    MemoryInfo* getMemoryInfo();

    void endBlock();
};


#endif //SHOOTERGAME_PROFILER_H
