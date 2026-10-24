#include "Threader.h"

int Threader::gThreadDelay = 10;
std::atomic<int> Threader::activeThreads(0);
std::mutex Threader::m;
std::mutex Threader::cleanupMutex;
bool Threader::stop = false;
std::condition_variable Threader::cv;
std::queue<std::pair<std::string, std::function<void(std::string)>>> Threader::taskQueue;
std::vector<std::thread> Threader::workers;

void Threader::worker()
{
    while (true) {
        std::pair<std::string, std::function<void(std::string)>> task;
        {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock, [] { return stop || !taskQueue.empty(); });
            if (stop && taskQueue.empty()) return;
            task = std::move(taskQueue.front());
            taskQueue.pop();
        }

        ++activeThreads;
        task.second(task.first);
        --activeThreads;
    }
}

void Threader::fireThread(std::string ip, void (*func)(std::string))
{
    {
        std::lock_guard<std::mutex> lock(m);
        if (workers.empty()) {
            stop = false;
            int numThreads = gThreads;
            if (numThreads <= 0) numThreads = 1;
            for (int i = 0; i < numThreads; ++i) {
                workers.emplace_back(worker);
            }
        }
        taskQueue.push(std::make_pair(std::move(ip), func));
    }
    cv.notify_one();
    Sleep(gThreadDelay);
}

void Threader::cleanUp()
{
    std::lock_guard<std::mutex> cleanupLock(cleanupMutex);
    {
        std::lock_guard<std::mutex> lock(m);
        if (workers.empty()) {
            stop = false;
            return;
        }
        stop = true;
        std::queue<std::pair<std::string, std::function<void(std::string)>>> empty;
        std::swap(taskQueue, empty);
    }
    cv.notify_all();
    for (auto &w : workers) {
        if (w.joinable()) w.join();
    }
    workers.clear();
    activeThreads = 0;
    stop = false;
}
