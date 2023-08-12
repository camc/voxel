#include <mgr/threadpool.h>

using namespace mgr;

// could use std::jthread?

// Special job detected by the thread telling it to stop
static void STOPPER() { throw std::logic_error("thread pool stopper should never be called"); }

void ThreadPool::thread_main() {
    while (true) {
        std::function<void()> job;

        {
            std::unique_lock<std::mutex> lock(jobs_mutex);
            jobs_cv.wait(lock, [this] { return !jobs.empty(); });

            job = jobs.front();

            // Check if the job is the stopper
            void (*const* target_ptr)(void) = job.target<void (*)(void)>();
            if (target_ptr && *target_ptr == STOPPER) {
                return;
            }

            jobs.pop();
        }

        job();
    }
}

ThreadPool::ThreadPool(size_t threads) {
    if (threads == 0) {
        throw std::invalid_argument("thread pool must have at least one thread");
    }

    for (size_t i = 0; i < threads; i++) {
        this->threads.emplace_back(&ThreadPool::thread_main, this);
    }
}

void ThreadPool::enqueue(const std::function<void()>& job) {
    std::scoped_lock<std::mutex> lock(jobs_mutex);

    if (threads.size() == 0) {
        throw std::runtime_error("attempt to enqueue job on stopped thread pool");
    }

    jobs.push(job);
    jobs_cv.notify_one();
}

void ThreadPool::enqueue(std::span<const std::function<void()>> jobs_todo) {
    std::scoped_lock<std::mutex> lock(jobs_mutex);

    if (threads.size() == 0) {
        throw std::runtime_error("attempt to enqueue job on stopped thread pool");
    }

    for (const auto& job : jobs_todo) {
        jobs.push(job);
        jobs_cv.notify_one();
    }
}

void ThreadPool::stop() {
    std::vector<std::thread> moved_threads;

    {
        std::scoped_lock<std::mutex> lock(jobs_mutex);

        if (threads.size() == 0) {
            return;
        }

        // Add a special job to the queue to tell the threads to stop
        jobs = {};
        jobs.push(STOPPER);

        moved_threads = std::move(threads);
        threads.clear();
    }

    // Wake up all threads
    jobs_cv.notify_all();

    // Wait for all threads to stop
    for (auto& thread : moved_threads) {
        thread.join();
    }

    jobs = {};
}