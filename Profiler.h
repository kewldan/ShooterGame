#ifndef SHOOTERGAME_PROFILER_H
#define SHOOTERGAME_PROFILER_H

#include <map>
#include <string>
#include <chrono>

struct ProfilerBlock {
    unsigned long allTime;
    unsigned int iterations;
};

class Profiler {
    std::string lastBlockName;
    unsigned long lastBlockStart;
public:
    std::map<std::string, ProfilerBlock> blocks;

    Profiler();

    void startBlock(std::string name);

    void endBlock();
};


#endif //SHOOTERGAME_PROFILER_H
