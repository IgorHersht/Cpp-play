#include <iostream>
#include <thread>
#include <sstream>
#include <string>

// Each thread will have its own 'counter' initialized to 0
thread_local int counter = 0;

void increment(int id) {
    counter++;
    std::cout << "Thread " << id << " counter: " << counter << "\n";
}

struct Entry {
    inline static thread_local  std::string name;
    void operator()() {
        auto id = std::this_thread::get_id();
        std::ostringstream oss;
        oss << id;
        name = oss.str();
        std::cout << name <<std::endl;
    }
};


int main() {
    std::jthread t1(increment, 1);
    std::jthread t2(increment, 2);
    Entry e1, e2;
    std::jthread t3(e1);
    std::jthread t4(e2);
}

/*
*Thread 1 counter: 1
135134332311232
135134323918528
Thread 2 counter: 1
*/
