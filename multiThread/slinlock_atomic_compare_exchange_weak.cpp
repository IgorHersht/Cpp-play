#include <atomic>

struct Spinlock {
    std::atomic<bool> lock_flag{false};

    void lock() {
        bool expected = false;
        // 1. Loop until the compare-exchange succeeds
        // 2. We reset 'expected' to false if it was changed by a failed attempt
        //bool compare_exchange_strong (T& expected, T desired)
        while (!lock_flag.compare_exchange_weak(expected, true, 
                                                std::memory_order_acquire, 
                                                std::memory_order_relaxed)) {
            expected = false; 
        }
    }

    void unlock() {
        // Use release semantics to ensure previous writes are visible to the next owner
        lock_flag.store(false, std::memory_order_release);
    }
};
