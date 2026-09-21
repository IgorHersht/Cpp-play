#include <atomic>
#include <cstdint>
#include <thread>

class spin_ticket_mutex {
public:
    void lock() {
        const std::uint64_t my_ticket =
            next_ticket_.fetch_add(1, std::memory_order_relaxed);

        while (serving_.load(std::memory_order_acquire) != my_ticket) {
            std::this_thread::yield();
        }
    }

    bool try_lock() {
        std::uint64_t current = serving_.load(std::memory_order_acquire);
        std::uint64_t expected = current;

        return next_ticket_.compare_exchange_strong(
            expected,
            current + 1,
            std::memory_order_acquire,
            std::memory_order_relaxed
        );
    }

    void unlock() {
        serving_.fetch_add(1, std::memory_order_release);
    }

private:
    alignas(hardware_destructive_interference_size) std::atomic<std::uint64_t> next_ticket_{ 0 };
    alignas(hardware_destructive_interference_size) std::atomic<std::uint64_t> serving_{ 0 };
};
