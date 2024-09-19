#include "../include/ThreadPool.hpp"


void ThreadPool::start(size_t num_threads) {
    num_threads = std::min((size_t)std::thread::hardware_concurrency(), num_threads+threads.size());
    threads.reserve(num_threads);
    for (size_t i=threads.size(); i<num_threads; ++i)
        threads.emplace_back(std::thread([this] {
            ThreadPool::ThreadLoop();
        }));
}


void ThreadPool::ThreadLoop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lck(q_mtx);

            start_new_job_cond.wait(lck, [this] {
                return !jobs.empty() || command_terminate;
            });

            if (command_terminate)
                return;

            job = jobs.front();
            jobs.pop();
        }

        job();
    }
}


void ThreadPool::enq_job(const std::function<void()>& job) {
    {
        std::unique_lock<std::mutex> lck(q_mtx);
        jobs.push(job);
    }
    start_new_job_cond.notify_one();
}


bool ThreadPool::is_busy() {
    std::unique_lock<std::mutex> lck(q_mtx);
    
    return !jobs.empty();
}


void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(q_mtx);
        command_terminate = true;
    }

    start_new_job_cond.notify_all();
    for (std::thread& thread : threads)
        thread.join();

    threads.clear();
}