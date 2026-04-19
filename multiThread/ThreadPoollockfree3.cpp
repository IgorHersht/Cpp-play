#include <cstddef>
#include <utility>
#include <new>
#include <stop_token>
#include <type_traits>

class SmallTask {
    static constexpr std::size_t SIZE = 64;
    static constexpr std::size_t ALIGN = alignof(std::max_align_t);

    using Storage = std::aligned_storage_t<SIZE, ALIGN>;

    using Fn = void(*)(void*, std::stop_token);
    using MoveFn = void(*)(void*, void*);
    using DestroyFn = void(*)(void*);

    Storage storage_;
    Fn fn_ = nullptr;
    MoveFn move_ = nullptr;
    DestroyFn destroy_ = nullptr;

public:
    SmallTask() = default;

    template <typename F>
    SmallTask(F&& f) {
        using T = std::decay_t<F>;
        static_assert(sizeof(T) <= SIZE, "Task too large for SBO");

        new (&storage_) T(std::forward<F>(f));

        fn_ = [](void* p, std::stop_token st) {
            (*reinterpret_cast<T*>(p))(st);
            };

        move_ = [](void* src, void* dst) {
            new (dst) T(std::move(*reinterpret_cast<T*>(src)));
            };

        destroy_ = [](void* p) {
            reinterpret_cast<T*>(p)->~T();
            };
    }

    SmallTask(SmallTask&& other) noexcept {
        move_from(std::move(other));
    }

    SmallTask& operator=(SmallTask&& other) noexcept {
        if (this != &other) {
            reset();
            move_from(std::move(other));
        }
        return *this;
    }

    ~SmallTask() { reset(); }

    void operator()(std::stop_token st) {
        fn_(&storage_, st);
    }

    explicit operator bool() const { return fn_ != nullptr; }

private:
    void move_from(SmallTask&& other) {
        fn_ = other.fn_;
        move_ = other.move_;
        destroy_ = other.destroy_;

        if (fn_) {
            move_(&other.storage_, &storage_);
            other.reset();
        }
    }

    void reset() {
        if (fn_) {
            destroy_(&storage_);
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
    explicit RingQueue(size_t size)
        : size_(size), buffer_(size)
    {
    }

    bool push(T&& item) {
        auto tail = tail_.load(std::memory_order_relaxed);
        auto next = (tail + 1) % size_;

        if (next == head_.load(std::memory_order_acquire))
            return false; // full

        buffer_[tail] = std::move(item);
        tail_.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        auto head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire))
            return false; // empty

        item = std::move(buffer_[head]);
        head_.store((head + 1) % size_, std::memory_order_release);
        return true;
    }

private:
    size_t size_;
    std::vector<T> buffer_;
    std::atomic<size_t> head_{ 0 };
    std::atomic<size_t> tail_{ 0 };
};

#include <vector>
#include <thread>
#include <semaphore>
#include <future>
#include <stop_token>

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

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
    {
        using R = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        auto fut = task->get_future();

        SmallTask wrapper{
            [task](std::stop_token) {
                (*task)();
            }
        };

        while (!queue_.push(std::move(wrapper))) {
            std::this_thread::yield(); // or backoff
        }

        sem_.release();
        return fut;
    }

private:
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