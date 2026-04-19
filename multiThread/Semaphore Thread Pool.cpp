#include <vector>
#include <queue>
#include <thread>
#include <future>
#include <functional>
#include <semaphore>
#include <mutex>
#include <type_traits>
#include <stdexcept>

class ThreadPool {
public:
    explicit ThreadPool(std::size_t n)
        : sem_(0)
    {
        workers_.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            workers_.emplace_back(
                [this](std::stop_token st) {
                    worker_loop(st);
                }
            );
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

        // wake all threads
        sem_.release(workers_.size());
        // jthread auto-joins
    }

    // =========================
    // SUBMIT (returns future)
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

        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (stopping_) {
                throw std::runtime_error("submit on stopped ThreadPool");
            }

            tasks_.emplace([task](std::stop_token st) {
                (*task)(st);
                });
        }

        sem_.release();
        return fut;
    }

    // =========================
    // SUBMIT (fire-and-forget)
    // =========================
    template <class F, class... Args>
    void submit_detach(F&& f, Args&&... args)
    {
        auto bound = bind_with_optional_stop(
            std::forward<F>(f),
            std::forward<Args>(args)...
        );

        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (stopping_) {
                throw std::runtime_error("submit on stopped ThreadPool");
            }

            tasks_.emplace(std::move(bound));
        }

        sem_.release();
    }

private:
    // =========================
    // Worker loop
    // =========================
    void worker_loop(std::stop_token st) {
        while (true) {
            sem_.acquire();

            if (st.stop_requested()) {
                std::lock_guard<std::mutex> lk(mtx_);
                if (tasks_.empty())
                    return;
            }

            std::function<void(std::stop_token)> task;

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
    // Helper: inject stop_token
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
        }
        else {
            return [f = std::forward<F>(f),
                ... args = std::forward<Args>(args)]
                (std::stop_token) mutable {
                return std::invoke(f, args...);
                };
        }
    }

private:
    std::vector<std::jthread> workers_;
    std::queue<std::function<void(std::stop_token)>> tasks_;

    std::mutex mtx_;
    std::counting_semaphore<> sem_;

    bool stopping_ = false;
};

#include <iostream>
int main() {
    ThreadPool pool(4);

    auto f1 = pool.submit([] { return 42; });
    auto f2 = pool.submit([](int x) { return x * 2; }, 21);

    std::cout << f1.get() << "\n"; // 42
    std::cout << f2.get() << "\n"; // 42
}