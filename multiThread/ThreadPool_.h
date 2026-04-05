#pragma once

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
            _workers.emplace_back([this](std::stop_token st) {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(_mutex);
                        // wait for stop request st or _tasks not empty
                        if (!_cv.wait(lock, st, [this] { return !_tasks.empty(); })) {
                            return; // Stop requested and queue is handled
                        }

                        if (st.stop_requested() && _tasks.empty()) {
                            return;
                        }

                        if (!_tasks.empty()) {
                            task = std::move(_tasks.front());
                            _tasks.pop();
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
            std::lock_guard lock(_mutex);
            _tasks.emplace([task]() { (*task)(); });
        }
        _cv.notify_one();
        return res;
    }

private:
    std::queue<std::function<void()>> _tasks;
    std::mutex _mutex;
    std::condition_variable_any _cv;
    std::vector<std::jthread> _workers;// must be the last data member (otherwise need destructor with _workers.clear())
};