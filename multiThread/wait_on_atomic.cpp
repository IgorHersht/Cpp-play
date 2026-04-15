#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

using namespace std::literals;

int main()
{
    std::atomic<bool> all_tasks_completed{false};
    std::atomic<unsigned> completion_count{};
    std::future<void> task_futures[16];
    std::atomic<unsigned> outstanding_task_count{16};

    // Spawn several tasks which take different amounts of
    // time, then decrement the outstanding task count.
    for (std::future<void>& task_future : task_futures)
        task_future = std::async([&]
        {
            // This sleep represents doing real work...
            std::this_thread::sleep_for(50ms);

            ++completion_count;
            --outstanding_task_count;

            // When the task count falls to zero, notify
            // the waiter (main thread in this case).
            if (outstanding_task_count.load() == 0)
            {
                // set and signal
                all_tasks_completed = true;
                all_tasks_completed.notify_one();
            }
        });

//     Performs atomic waiting operations. Behaves as if it repeatedly performs the following steps:
//
// Compare the value representation of this->load(order) with that of old.
// If those are equal, then blocks until *this is notified by notify_one() or notify_all(), or the thread is unblocked spuriously.
// Otherwise, returns.
// These functions are guaranteed to return only if value has changed, even if underlying implementation unblocks spuriously.

    all_tasks_completed.wait(false);

    std::cout << "Tasks completed = " << completion_count.load() << '\n';
}
