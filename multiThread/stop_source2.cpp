#include <iostream>
#include <stop_token>
#include <future>     // for std::async()
#include <thread>     // for sleep_for()
#include <syncstream> // for std::osyncstream
#include <chrono>
using namespace std::literals;  // for duration literals

//std::stop_source is safe to use from multiple threads.You can copy or move a stop_source so multiple objects share the same stop state; any copy can call request_stop() to signal cancellation for all observers that hold the associated std::stop_token.
//Concurrent calls to request_stop(), get_token(), and queries via std::stop_token::stop_requested() are allowed; the library synchronizes access to the shared stop state.
//std::stop_callback registration is also safe : callbacks will be invoked(synchronously or asynchronously per implementation) when stop is requested.
//Minimal example showing sharing :

void worker(std::stop_token st, int id) {
    while (!st.stop_requested()) { /* work */ }
    std::cout << "worker " << id << " stopped\n";
}

int main() {
    std::stop_source src;
    auto src_copy = src;                 // shares same stop state
    std::thread t1(worker, src.get_token(), 1);
    std::thread t2(worker, src_copy.get_token(), 2);

    // request stop from another thread or copy
    std::thread controller([src_copy]() mutable {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            src_copy.request_stop();
    });

    t1.join(); t2.join(); controller.join();
}
