#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            // jthread automatically passes its internal stop_token to the lambda
            workers.emplace_back([this](std::stop_token st) {
                while (!st.stop_requested()) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(queue_mutex);
                        // wait for stop request st or tasks not empty
                        if (!cv.wait(lock, st, [this] { return !tasks.empty(); })) {
                            return; // Stop requested and queue is handled
                        }
                        if (!tasks.empty()) {
                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                    }
                    if (task) {
                        task();
                    }
                }
                });
        }
    }

    // Submit a task and return a future
    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> res = task->get_future();
        {
            std::lock_guard lock(queue_mutex);
            tasks.emplace([task]() { (*task)(); });
        }
        cv.notify_one();
        return res;
    }

private:
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable_any cv;
    std::vector<std::jthread> workers;// must be the last data member (otherwise need destructor with workers.clear())
};

int main() {
    {
        ThreadPool pool(4); // Pool with 4 threads

        // Test 1: Simple task
        pool.enqueue([] {
            std::cout << "Task 1 executing on thread " << std::this_thread::get_id() << std::endl;
            });

        // Test 2: Task with return value
        auto result = pool.enqueue([](int a, int b) {
            return a + b;
            }, 10, 20);

        std::cout << "Result of 10 + 20: " << result.get() << std::endl;

        // Test 3: Multiple rapid tasks
        for (int i = 0; i < 5; ++i) {
            pool.enqueue([i] {
                std::cout << "Bulk task " << i << " running..." << std::endl;
                });
        }
    } // Pool goes out of scope here: all jthreads stop and join automatically

    std::cout << "ThreadPool destroyed and all threads joined." << std::endl;
    return 0;
}


