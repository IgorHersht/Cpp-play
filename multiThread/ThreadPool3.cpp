#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    template<typename F>
    void enqueue(F&& f);

    void stop(); // Stop all threads

private:
    std::vector<std::jthread> workers; // Use jthread for automatic join on destruction
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop_flag{false}; // Atomic flag to indicate stopping state
};

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this](std::stop_token stopToken) {
            while (!stopToken.stop_requested()) { // Check if stop is requested
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this, &stopToken] {
                        return stop_flag.load() || !tasks.empty() || stopToken.stop_requested();
                    });
                    if (stop_flag.load() && tasks.empty())
                        return; // Exit if stop is requested and no tasks are left
                    if (!tasks.empty()) {
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                }
                if (task) {
                    task(); // Execute the task
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop(); // Ensure that stop is called on destruction
}

template<typename F>
void ThreadPool::enqueue(F&& f) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(std::forward<F>(f)); // Store the task
    }
    condition.notify_one(); // Notify one waiting thread
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_flag.store(true); // Set the flag to indicate stopping
    }
    condition.notify_all(); // Wake all threads to finish
}

int main() {
    ThreadPool pool(4); // Create a thread pool with 4 threads

    // Enqueue tasks
    for (int i = 0; i < 10; ++i) {
        pool.enqueue([i] {
            std::cout << "Processing task " << i << " on thread "
                      << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
        });
    }

    // Allow some time for tasks to be processed
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop the thread pool
    pool.stop();

    return 0;
}

