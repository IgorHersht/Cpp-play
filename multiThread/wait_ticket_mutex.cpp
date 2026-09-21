#include <atomic>
#include <new>

struct wait_ticket_mutex {
private:
    alignas(std::hardware_destructive_interference_size)
        std::atomic<int> in{ 0 };
    alignas(std::hardware_destructive_interference_size)
        std::atomic<int> out{ 0 };
public:
    void lock() {
        auto const my = in.fetch_add(1, std::memory_order_relaxed);
        while (true) {
            auto const now = out.load(std::memory_order_acquire);
            if (now == my) {
                return;
            }
            out.wait(now, std::memory_order_relaxed);
        }
    }

    bool try_lock() {
        auto current = out_.load(std::memory_order_acquire);
        auto expected = current;

        return in_.compare_exchange_strong(
            expected,
            current + 1,
            std::memory_order_acquire,
            std::memory_order_relaxed
        );
    }

    void unlock() {
        out.fetch_add(1, std::memory_order_release);
        out.notify_all();
    }
};
