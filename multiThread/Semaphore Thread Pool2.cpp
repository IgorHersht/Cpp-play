#include <vector>
#include <queue>
#include <thread>
#include <future>
#include <semaphore>
#include <mutex>
#include <memory>
#include <type_traits>
#include <utility>
#include <stdexcept>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t n)
        : sem_(0)
    {
        workers_.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            workers_.emplace_back([this](std::stop_token st) {
                worker_loop(st);
            });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stopping_ = true;
        }

        for (auto& t : workers_) {
            t.request_stop();
        }

        sem_.release(workers_.size());
    }

    // =========================
    // submit -> future<T>
    // =========================
    template <class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using R = std::invoke_result_t<F, Args...>;

        auto bound = bind_with_optional_stop(
            std::forward<F>(f),
            std::forward<Args>(args)...
        );

        auto task = std::make_shared<std::packaged_task<R(std::stop_token)>>(
            std::move(bound)
        );

        std::future<R> fut = task->get_future();

        enqueue(Task{
            [task](std::stop_token st) {
                (*task)(st);
            }
        });

        return fut;
    }

    // =========================
    // fire-and-forget
    // =========================
    template <class F, class... Args>
    void submit_detach(F&& f, Args&&... args)
    {
        auto bound = bind_with_optional_stop(
            std::forward<F>(f),
            std::forward<Args>(args)...
        );

        enqueue(Task{std::move(bound)});
    }

private:
    // =========================
    // Lightweight move-only task
    // =========================
    struct Task {
        using Fn = void(*)(void*, std::stop_token);

        void* obj = nullptr;
        Fn fn = nullptr;
        void (*deleter)(void*) = nullptr;

        Task() = default;

        template <class F>
        Task(F&& f) {
            using T = std::decay_t<F>;
            T* ptr = new T(std::forward<F>(f));

            obj = ptr;
            fn = [](void* p, std::stop_token st) {
                (*static_cast<T*>(p))(st);
            };
            deleter = [](void* p) {
                delete static_cast<T*>(p);
            };
        }

        Task(Task&& other) noexcept
            : obj(other.obj), fn(other.fn), deleter(other.deleter)
        {
            other.obj = nullptr;
        }

        Task& operator=(Task&& other) noexcept {
            if (this != &other) {
                reset();
                obj = other.obj;
                fn = other.fn;
                deleter = other.deleter;
                other.obj = nullptr;
            }
            return *this;
        }

        ~Task() { reset(); }

        void operator()(std::stop_token st) {
            fn(obj, st);
        }

        explicit operator bool() const {
            return obj != nullptr;
        }

    private:
        void reset() {
            if (obj) {
                deleter(obj);
                obj = nullptr;
            }
        }

        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;
    };

    void enqueue(Task&& task) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (stopping_) {
                throw std::runtime_error("submit on stopped ThreadPool");
            }
            tasks_.push(std::move(task));
        }
        sem_.release();
    }

    void worker_loop(std::stop_token st) {
        while (true) {
            sem_.acquire();

            if (st.stop_requested()) {
                std::lock_guard<std::mutex> lk(mtx_);
                if (tasks_.empty())
                    return;
            }

            Task task;

            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (tasks_.empty())
                    continue;

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            task(st);
        }
    }

    // =========================
    // stop_token injection
    // =========================
    template <class F, class... Args>
    static auto bind_with_optional_stop(F&& f, Args&&... args)
    {
        if constexpr (std::is_invocable_v<F, std::stop_token, Args...>) {
            return [f = std::forward<F>(f),
                    ... args = std::forward<Args>(args)]
                   (std::stop_token st) mutable {
                return std::invoke(f, st, args...);
            };
        } else {
            return [f = std::forward<F>(f),
                    ... args = std::forward<Args>(args)]
                   (std::stop_token) mutable {
                return std::invoke(f, args...);
            };
        }
    }

private:
    std::vector<std::jthread> workers_;
    std::queue<Task> tasks_;

    std::mutex mtx_;
    std::counting_semaphore<> sem_;
    bool stopping_ = false;
};