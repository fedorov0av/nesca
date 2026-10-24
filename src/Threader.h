#ifndef THREADER_H
#define THREADER_H

#include "mainResources.h"
#include "externData.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

class Threader {

public:
    static std::atomic<int> activeThreads;
    static std::mutex m;
    static std::mutex cleanupMutex;
    static bool stop;
    static std::condition_variable cv;
    static std::queue<std::pair<std::string, std::function<void(std::string)>>> taskQueue;
    static int gThreadDelay;
    static std::vector<std::thread> workers;

public:
    static void fireThread(std::string ip, void (*func)(std::string));
    static void cleanUp();
    static void worker();
};

#endif // THREADER_H
