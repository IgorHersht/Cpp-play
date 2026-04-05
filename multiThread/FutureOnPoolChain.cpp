

#include <ThreadPool.h>

template< typename PoolT, typename F, typename... Args>
auto futureOnPool( PoolT& pool, F&& f, Args&&... args) {
    return pool.submitAndGetFuture( std::forward<F>(f), std::forward<Args>(args)...);
}

// test
#include <iostream>
#include <cassert>

int foo1(int i){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return i + 1;
}

int foo2(int i){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return i + 2;
}

int foo3(int i){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return i + 3;
}

int foo1Andfoo2_foo3(ThreadPool& pool,int i1, int i2){
    std::shared_future<int> f1 = futureOnPool(pool,foo1, i1);
    std::shared_future<int> f2 = futureOnPool(pool,foo2, i2);
    std::future<int> f3 = futureOnPool(pool, [f1 = std::move(f1), f2 = std::move(f2)]{
        return  foo3( f1.get() + f2.get() );
    });
    int i = f3.get();
    return i;
}

int main() {
    ThreadPool pool = ThreadPool(27);
    int i = foo1Andfoo2_foo3(pool, 5, 7);
    assert(i == 18);

    return 0;
}

