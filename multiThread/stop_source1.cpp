#include <iostream>
#include <stop_token>
#include <future>     // for std::async()
#include <thread>     // for sleep_for()
#include <syncstream> // for std::osyncstream
#include <chrono>
using namespace std::literals;  // for duration literals


//The signaler.Holds shared state and can request cancellation via request_stop().
//Produces std::stop_token by get_token().
//Multiple stop_source objects can share the same state(copy / move semantics depend on implementation), but typically you treat one source as the controller.
//You call request_stop() when you want to signal cancellation.
//std::stop_token
//
//The observer.Lightweight handle to the stop state obtained from a stop_source(or from std::jthread::get_stop_token()).
//Can query stop_requested() and stop_possible().
//Can register callbacks(std::stop_callback) that are invoked if / when stop is requested.
//Multiple tokens can observe the same stop state.
//When to use which :
//
//Use stop_source where you need to initiate / call cancellation.
//Pass stop_token to workers / receivers so they can observe or react to cancellation.
//Minimal example :


void worker(std::stop_token st)
{
    while (!st.stop_requested()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "working...\n";
    }
    std::cout << "stopped\n";
}

int main()
{
    std::stop_source src;
    std::stop_token token = src.get_token();


        std::thread t(worker, token);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    src.request_stop(); // signal cancellation

    t.join();
}

