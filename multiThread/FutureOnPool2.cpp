
#include <ThreadPool.h>

template< typename PoolT, typename F, typename... Args>
auto futureOnPool( PoolT& pool, F&& f, Args&&... args) {
    return pool.submitAndGetFuture( std::forward<F>(f), std::forward<Args>(args)...);
}

// test
#include <iostream>

int main() {
    std::vector<std::future<int>> results;
    {
        ThreadPool pool = ThreadPool(27);
        // Test 1: Simple task
        futureOnPool(pool, [] {
            std::cout << "Task 1 executing on thread " << std::this_thread::get_id() << std::endl;
            });

        // Test 2: Task with return value
        for (int i = 0; i < 10; ++i) {
            auto result = pool.submitAndGetFuture([](int a, int b) {
             return a + b;
             }, i, 2 * i);
            results.emplace_back(std::move(result));
        }

        for (int i = 0; i < 10; ++i) {
            auto result = futureOnPool(pool, [] (int a, int b) {
             return a + b;
             }, i, 2 * i);
            results.emplace_back(std::move(result));
        }
    } // Pool goes out of scope here: all jthreads stop and join automatically

    std::cout << "ThreadPool destroyed and all threads joined." << std::endl;
    for (auto& result : results) {
        std::cout << "Result of task: " << result.get() << std::endl;
    }
    return 0;
}

