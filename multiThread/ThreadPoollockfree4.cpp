#include <cstddef>
#include <utility>
#include <new>
#include <stop_token>
#include <type_traits>

class SmallTask {
    static constexpr size_t SIZE = 64;
    alignas(std::max_align_t) unsigned char storage[SIZE];

    using Fn = void(*)(void*, std::stop_token);
    using DestroyFn = void(*)(void*);

    Fn fn_ = nullptr;
    DestroyFn destroy_ = nullptr;

public:
    SmallTask() = default;

    template<typename F>
    SmallTask(F&& f) {
        using T = std::decay_t<F>;
        static_assert(sizeof(T) <= SIZE, "Task too large");

        new (storage) T(std::forward<F>(f));

        fn_ = [](void* p, std::stop_token st) {
            (*reinterpret_cast<T*>(p))(st);
            };

        destroy_ = [](void* p) {
            reinterpret_cast<T*>(p)->~T();
            };
    }

    SmallTask(SmallTask&& other) noexcept {
        std::memcpy(storage, other.storage, SIZE);
        fn_ = other.fn_;
        destroy_ = other.destroy_;
        other.fn_ = nullptr;
    }

    SmallTask& operator=(SmallTask&& other) noexcept {
        if (this != &other) {
            reset();
            std::memcpy(storage, other.storage, SIZE);
            fn_ = other.fn_;
            destroy_ = other.destroy_;
            other.fn_ = nullptr;
        }
        return *this;
    }

    ~SmallTask() { reset(); }

    void operator()(std::stop_token st) {
        fn_(storage, st);
    }

    explicit operator bool() const { return fn_ != nullptr; }

private:
    void reset() {
        if (fn_) {
            destroy_(storage);
            fn_ = nullptr;
        }
    }

    SmallTask(const SmallTask&) = delete;
    SmallTask& operator=(const SmallTask&) = delete;
};

#include <atomic>
#include <vector>

template<typename T>
class RingQueue {
public:
    explicit RingQueue(size_t capacity)
        : capacity_(capacity), buffer_(capacity) {
    }

    bool push(T&& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next = (tail + 1) % capacity_;

        if (next == head_.load(std::memory_order_acquire))
            return false; // full

        buffer_[tail] = std::move(item);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire))
            return false; // empty

        item = std::move(buffer_[head]);
        head_.store((head + 1) % capacity_, std::memory_order_release);
        return true;
    }

private:
    size_t capacity_;
    std::vector<T> buffer_;
    std::atomic<size_t> head_{ 0 };
    std::atomic<size_t> tail_{ 0 };
};

#include <memory>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T>
struct SharedState {
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;
    std::optional<T> value;

    void set(T v) {
        {
            std::lock_guard lk(m);
            value = std::move(v);
            ready = true;
        }
        cv.notify_all();
    }
};

template<>
struct SharedState<void> {
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;

    void set() {
        {
            std::lock_guard lk(m);
            ready = true;
        }
        cv.notify_all();
    }
};

template<typename T>
class SimpleFuture {
    std::shared_ptr<SharedState<T>> state_;
public:
    explicit SimpleFuture(std::shared_ptr<SharedState<T>> s)
        : state_(std::move(s)) {
    }

    T get() {
        std::unique_lock lk(state_->m);
        state_->cv.wait(lk, [&] { return state_->ready; });
        return std::move(*state_->value);
    }
};

template<>
class SimpleFuture<void> {
    std::shared_ptr<SharedState<void>> state_;
public:
    explicit SimpleFuture(std::shared_ptr<SharedState<void>> s)
        : state_(std::move(s)) {
    }

    void get() {
        std::unique_lock lk(state_->m);
        state_->cv.wait(lk, [&] { return state_->ready; });
    }
};

#include <vector>
#include <thread>
#include <semaphore>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t threads, size_t queue_size = 1024)
        : queue_(queue_size), sem_(0)
    {
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this](std::stop_token st) {
                worker_loop(st);
                });
        }
    }

    ~ThreadPool() {
        for (auto& t : workers_)
            t.request_stop();

        sem_.release(workers_.size());
    }

    // =========================
    // submit -> future
    // =========================
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
    {
        using R = std::invoke_result_t<F, Args...>;

        auto state = std::make_shared<SharedState<R>>();

        SmallTask task{
            [state, f = std::forward<F>(f), ...args = std::forward<Args>(args)]
            (std::stop_token) mutable {
                if constexpr (std::is_void_v<R>) {
                    std::invoke(f, args...);
                    state->set();
                }
 else {
  state->set(std::invoke(f, args...));
}
}
        };

        enqueue(std::move(task));
        return SimpleFuture<R>(state);
    }

    // =========================
    // submit_detach -> void
    // =========================
    template<typename F, typename... Args>
    void submit_detach(F&& f, Args&&... args)
    {
        SmallTask task{
            [f = std::forward<F>(f), ...args = std::forward<Args>(args)]
            (std::stop_token) mutable {
                std::invoke(f, args...);
            }
        };

        enqueue(std::move(task));
    }

private:
    void enqueue(SmallTask task) {
        while (!queue_.push(std::move(task))) {
            std::this_thread::yield(); // backpressure
        }
        sem_.release();
    }

    void worker_loop(std::stop_token st) {
        while (true) {
            sem_.acquire();

            if (st.stop_requested())
                return;

            SmallTask task;
            if (queue_.pop(task)) {
                task(st);
            }
        }
    }

private:
    std::vector<std::jthread> workers_;
    RingQueue<SmallTask> queue_;
    std::counting_semaphore<> sem_;
};

int main() {}