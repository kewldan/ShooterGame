#include "MyProfiler.h"

MyProfiler::MyProfiler() {
    lastBlockStart = 0L;
}

void MyProfiler::startBlock(const char* name) {
    lastBlockName = name;
    lastBlockStart = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
}

void MyProfiler::endBlock() {
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


