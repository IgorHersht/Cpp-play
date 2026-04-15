include <atomic>

struct Spinlock {
    std::atomic<bool> locked{false};

    void lock() {
        // Exchange with true; if it returns true, the lock was already held.
        // We continue spinning until exchange() returns false.
        while (locked.exchange(true, std::memory_order_acquire)) {
        }
    }

    void unlock() {
        // Simply set the lock back to false
        locked.store(false, std::memory_order_release);
    }
};
