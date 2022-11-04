#include "Profiler.h"

Profiler::Profiler() {
    lastBlockStart = 0L;
}

void Profiler::startBlock(std::string name) {
    lastBlockName = name;
    lastBlockStart = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

MemoryInfo* Profiler::getMemoryInfo()
{
    MemoryInfo* output = new MemoryInfo();

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);

    output->virtualMemoryTotal = memInfo.ullTotalPageFile;
    output->physicalMemoryTotal = memInfo.ullTotalPhys;

    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    output->virtualMemoryUsed = pmc.PrivateUsage;
    output->physicalMemoryUsed = pmc.WorkingSetSize;

    return output;
}

void Profiler::endBlock() {
    unsigned long duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() - lastBlockStart;
    if (blocks.contains(lastBlockName)) {
        blocks[lastBlockName].iterations++;
        blocks[lastBlockName].allTime += duration;
    } else {
        blocks[lastBlockName] = {
                duration, 1
        };
    }
}


