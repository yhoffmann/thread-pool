#pragma once


#include <stdlib.h>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <array>
#include <atomic>

class ThreadPool {
public:
    ThreadPool() {}
    ThreadPool(size_t num_threads);
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    ~ThreadPool();

    void start(size_t num_threads);
    void enq_job(const std::function<void()>& job);
    size_t get_num_active_tasks();
    bool is_busy();
    void await();
    void clear_jobs();
    void stop();

protected:
    std::vector<std::thread> m_threads;

    std::queue<std::function<void()>> m_jobs;
    bool m_command_stop = false;
    std::mutex m_jobs_mtx;
    std::condition_variable m_start_new_job;

    std::atomic<size_t> m_num_active_tasks;
    std::mutex m_busy_mtx;
    std::condition_variable m_all_tasks_done;

    void thread_loop();
}; // class ThreadPool


ThreadPool::ThreadPool(size_t num_threads)
    : m_num_active_tasks(0)
{
    start(num_threads);
}


ThreadPool::~ThreadPool() {
    stop();
}


void ThreadPool::start(size_t num_threads) {
    {
        std::unique_lock<std::mutex> jobs_lck(m_jobs_mtx);
        m_command_stop = false;
    }

    size_t num_hardware_threads = (size_t)std::thread::hardware_concurrency();
    m_threads.reserve(num_hardware_threads);

    num_threads = std::min(num_hardware_threads, num_threads+m_threads.size());
    for (size_t i=m_threads.size(); i<num_threads; ++i) {
        m_threads.emplace_back(std::thread([this] {
            thread_loop();
        }));
    }
}


void ThreadPool::thread_loop() {
    while (true) {
        std::function<void()> job;
        
        {
            std::unique_lock<std::mutex> jobs_lck(m_jobs_mtx);
            m_start_new_job.wait(jobs_lck, [this] {
                return !m_jobs.empty() || m_command_stop;
            });

            if (m_command_stop) {
                return;
            }

            job = m_jobs.front();
            m_jobs.pop();
        }

        ++m_num_active_tasks;
        job();
        --m_num_active_tasks;

        {
            std::unique_lock<std::mutex> busy_lck(m_busy_mtx);
            m_all_tasks_done.notify_all();
        }
    }
}


void ThreadPool::enq_job(const std::function<void()>& job) {
    {
        std::unique_lock<std::mutex> jobs_lck(m_jobs_mtx);
        m_jobs.push(job);
    }

    m_start_new_job.notify_one();
}


size_t ThreadPool::get_num_active_tasks() {
    return size_t(m_num_active_tasks);
}


bool ThreadPool::is_busy() {
    std::unique_lock<std::mutex> jobs_lck(m_jobs_mtx);

    return !m_jobs.empty() || bool(m_num_active_tasks);
}


void ThreadPool::await() {
    {
        std::unique_lock<std::mutex> jobs_lck(m_jobs_mtx);
        if (m_command_stop) {
            return;
        }
    }
    std::unique_lock<std::mutex> busy_lck(m_busy_mtx);
    m_all_tasks_done.wait(busy_lck, [this] {
        return !is_busy();
    });
}


void ThreadPool::clear_jobs() {
    stop();

    m_jobs = std::queue<std::function<void()>>();
}


void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> jobs_lck(m_jobs_mtx);
        if (m_command_stop) {
            return;
        }
        else {
            m_command_stop = true;
        }
    }

    m_start_new_job.notify_all();

    for (std::thread& thread : m_threads)
        thread.join();

    m_threads.clear();
}