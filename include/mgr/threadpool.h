#pragma once

#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <span>

namespace mgr {

// A fixed size thread pool (until stopped)
// The destructor will block until all threads have stopped.
class ThreadPool {
    std::queue<std::function<void()>> jobs;
    std::mutex jobs_mutex;
    std::condition_variable jobs_cv;
    std::vector<std::thread> threads;

    // Main function for each thread in the pool
    void thread_main();

    ThreadPool operator=(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&) = delete;

public:
    ThreadPool(size_t threads);
    ~ThreadPool() { stop(); }

    // Enqueues a job to be run by a thread in the pool.
    void enqueue(const std::function<void()>& job);

    // Enqueues a list of jobs to be run by threads in the pool.
    void enqueue(std::span<const std::function<void()>> jobs_todo);

    // Stops and destroys all threads in the pool, clearing any queued jobs.
    // Blocks until all threads have stopped.
    void stop();
};

}  // namespace mgr