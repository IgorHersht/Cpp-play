
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
/*
*bool wait( Lock& lock, std::stop_token stoken, Predicate pred )
* it is then equivalent to
while (!stoken.stop_requested())
{
   if (pred())
       return true;
   wait(lock);
}
return pred();
*/
template< class Lock, class Predicate >
bool waitAndStop(std::condition_variable_any& cv, Lock& lock, std::stop_token st, Predicate pred) {
    return !cv.wait(lock, st, pred);
}

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            // jthread automatically passes its internal stop_token to the lambda
            _workers.emplace_back([this](std::stop_token st) {
                while (true) { // would "drain" the task queue, use while(!st.stop_requested()) if don't want to drain
                    std::function<void()> task;
                    {
                        std::unique_lock lock(_mutex);
                        if (waitAndStop(_cv, lock, st, [this] { return !_tasks.empty(); })) {
                            return;
                        }
                        // !_tasks.empty() =-here
                        task = std::move(_tasks.front());
                        _tasks.pop();
                    }
                    task();
                }
                });
        }
    }

    // Submit a task and return a future
    //st.stop_requested() == true only on the ThreadPool destruction => cannot be called with st.stop_requested()
    template <typename F, typename... Args>
    auto submitAndGetFuture(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
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

    //st.stop_requested() == true only on the ThreadPool destruction => cannot be called with st.stop_requested()
    void submit(std::function<void()> f) {
        {
            std::unique_lock lock(_mutex);
            _tasks.emplace(f);
        }
        _cv.notify_one();
    }

private:
    std::queue<std::function<void()>> _tasks;
    std::mutex _mutex;
    std::condition_variable_any _cv;
    std::vector<std::jthread> _workers;//!!! must be the last data member (otherwise need destructor with _workers.clear())
};


// test
#include <iostream>

int main() {
    std::vector<std::future<int>> results;
    {
        ThreadPool pool = ThreadPool(27);


        // Test 1: Simple task
        pool.submitAndGetFuture([] {
            std::cout << "Task 1 executing on thread " << std::this_thread::get_id() << std::endl;
            });
        // Test 12: Simple task
        pool.submit([] {
            std::cout << "Task 12 executing on thread " << std::this_thread::get_id() << std::endl;
            });


        // Test 2: Task with return value
        for (int i = 0; i < 10; ++i) {
            auto result = pool.submitAndGetFuture([](int a, int b) {
             return a + b;
             }, i, 2 * i);
            results.emplace_back(std::move(result));
        }

        // Test 3: Multiple rapid _tasks
        for (int i = 0; i < 5; ++i) {
            pool.submitAndGetFuture([i] {
                std::cout << "Bulk task " << i << " running..." << std::endl;
                });
        }
    } // Pool goes out of scope here: all jthreads stop and join automatically

    std::cout << "ThreadPool destroyed and all threads joined." << std::endl;
    for (auto& result : results) {
        std::cout << "Result of task: " << result.get() << std::endl;
    }
    return 0;
}

