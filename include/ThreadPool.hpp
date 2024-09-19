#pragma once


#include <stdlib.h>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <array>

class ThreadPool {
public:
    ThreadPool() {}
    ThreadPool(size_t num_threads) { start(num_threads); }
    ThreadPool(const ThreadPool&) = default;
    ThreadPool& operator=(const ThreadPool&) = default;

    void start(size_t num_threads);
    void enq_job(const std::function<void()>& job);
    void stop();
    bool is_busy();

    template<uint Intrvl = 1000>
    void await() {
    while (is_busy())
        std::this_thread::sleep_for(std::chrono::milliseconds(Intrvl));
}

protected:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> jobs;
    bool command_terminate = false;
    std::mutex q_mtx;
    std::condition_variable start_new_job_cond;

    void ThreadLoop();
};