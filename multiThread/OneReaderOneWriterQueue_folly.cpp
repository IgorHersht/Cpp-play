
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

//#include <folly/concurrency/CacheLocality.h>

namespace folly {

  /*
   * ProducerConsumerQueue is a one producer and one consumer queue
   * without locks.
   *
  folly/ProducerConsumerQueue.h
  The folly::ProducerConsumerQueue class is a one-producer one-consumer queue with very low synchronization overhead.
  The queue must be created with a fixed maximum size (and allocates that many cells of sizeof(T)), and it provides just a few simple operations:
  read: Attempt to read the value at the front to the queue into a variable,
      returns `false` iff queue was empty.
  write: Emplace a value at the end of the queue, returns false iff the
       queue was full.
  frontPtr: Retrieve a pointer to the item at the front of the queue, or
          `nullptr` if it is empty.
  popFront: Remove the item from the front of the queue (queue must not be
          empty).
  isEmpty: Check if the queue is empty.
  isFull: Check if the queue is full.
  sizeGuess: Returns the number of entries in the queue. Because of the
           way we coordinate threads, this guess could be slightly wrong
           when called by the producer/consumer thread, and it could be
           wildly inaccurate if called from any other threads. Hence,
           only call from producer/consumer threads!
  All of these operations are wait-free. The read operations (including frontPtr and popFront) and write operations must only be called by the reader and writer thread, respectively. isFull, isEmpty,
   and sizeGuess may be called by either thread, but the return values from read, write, or frontPtr are sufficient for most cases.
  write may fail if the queue is full, and read may fail if the queue is empty, so in many situations it is important to choose the queue size such that the queue filling or staying empty for long is unlikely.
   */
template <class T>
struct ProducerConsumerQueue {
  using value_type = T;

  ProducerConsumerQueue(const ProducerConsumerQueue&) = delete;
  ProducerConsumerQueue& operator=(const ProducerConsumerQueue&) = delete;

  // size must be >= 2.
  //
  // Also, note that the number of usable slots in the queue at any
  // given time is actually (size-1), so if you start with an empty queue,
  // isFull() will return true after size-1 insertions.
  explicit ProducerConsumerQueue(uint32_t size)
      : size_(size),
        records_(static_cast<T*>(std::malloc(sizeof(T) * size))),
        readIndex_(0),
        writeIndex_(0) {
    assert(size >= 2);
    if (!records_) {
      throw std::bad_alloc();
    }
  }

  ~ProducerConsumerQueue() {
    // We need to destruct anything that may still exist in our queue.
    // (No real synchronization needed at destructor time: only one
    // thread can be doing this.)
    if (!std::is_trivially_destructible<T>::value) {
      size_t readIndex = readIndex_;
      size_t endIndex = writeIndex_;
      while (readIndex != endIndex) {
        records_[readIndex].~T();
        if (++readIndex == size_) {
          readIndex = 0;
        }
      }
    }

    std::free(records_);
  }

  template <class... Args>
  bool write(Args&&... recordArgs) {
    auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
    auto nextRecord = currentWrite + 1;
    if (nextRecord == size_) {
      nextRecord = 0;
    }
    if (nextRecord != readIndex_.load(std::memory_order_acquire)) {
      new (&records_[currentWrite]) T(std::forward<Args>(recordArgs)...);
      writeIndex_.store(nextRecord, std::memory_order_release);
      return true;
    }

    // queue is full
    return false;
  }

  // move (or copy) the value at the front of the queue to given variable
  bool read(T& record) {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
      // queue is empty
      return false;
    }

    auto nextRecord = currentRead + 1;
    if (nextRecord == size_) {
      nextRecord = 0;
    }
    record = std::move(records_[currentRead]);
    records_[currentRead].~T();
    readIndex_.store(nextRecord, std::memory_order_release);
    return true;
  }

  // pointer to the value at the front of the queue (for use in-place) or
  // nullptr if empty.
  T* frontPtr() {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
      // queue is empty
      return nullptr;
    }
    return &records_[currentRead];
  }

  // queue must not be empty
  void popFront() {
    auto const currentRead = readIndex_.load(std::memory_order_relaxed);
    assert(currentRead != writeIndex_.load(std::memory_order_acquire));

    auto nextRecord = currentRead + 1;
    if (nextRecord == size_) {
      nextRecord = 0;
    }
    records_[currentRead].~T();
    readIndex_.store(nextRecord, std::memory_order_release);
  }

  bool isEmpty() const {
    return readIndex_.load(std::memory_order_acquire) ==
        writeIndex_.load(std::memory_order_acquire);
  }

  bool isFull() const {
    auto nextRecord = writeIndex_.load(std::memory_order_acquire) + 1;
    if (nextRecord == size_) {
      nextRecord = 0;
    }
    if (nextRecord != readIndex_.load(std::memory_order_acquire)) {
      return false;
    }
    // queue is full
    return true;
  }

  // * If called by consumer, then true size may be more (because producer may
  //   be adding items concurrently).
  // * If called by producer, then true size may be less (because consumer may
  //   be removing items concurrently).
  // * It is undefined to call this from any other thread.
  size_t sizeGuess() const {
    int ret = writeIndex_.load(std::memory_order_acquire) -
        readIndex_.load(std::memory_order_acquire);
    if (ret < 0) {
      ret += size_;
    }
    return ret;
  }

  // maximum number of items in the queue.
  size_t capacity() const { return size_ - 1; }

 private:
  using AtomicIndex = std::atomic<unsigned int>;

  char pad0_[std::hardware_destructive_interference_size];
  const uint32_t size_;
  T* const records_;

  alignas(std::hardware_destructive_interference_size) AtomicIndex readIndex_;
  alignas(std::hardware_destructive_interference_size) AtomicIndex writeIndex_;

  char pad1_[std::hardware_destructive_interference_size - sizeof(AtomicIndex)];
};

} // namespace folly

using namespace folly;
//test
#include <vector>
#include <iostream>
#include <thread>
#include <functional>

    ProducerConsumerQueue<std::array<int64_t , 2> > q(256l);
    const  size_t capacity = q.capacity();
    using  El = std::array<int64_t, 2>;
     using Func = std::function<void(El)>;
    void onMessge(El el) {
        std::cout <<el[0] << ";" << el[1]<< std::endl;
    }

void push(El el){
    while(!q.write(el));
}

    void produce(){
        int64_t i = 0;
        while(true){
            El el {i, i + int64_t(capacity)};
            push(el);
            ++i;
        }
    }

    El pop() {
        El el {};
        while(!q.read(el));
        return el;
    }
    void consume(Func f ){
        El previusEl {}; // for testing
        while(true){
            El el = pop();
            f(el);
            //testing
            if(el[0]) { // not first
                assert( (previusEl[0] == el[0] -1) && (previusEl[1] == el[1] -1) );
            }
            previusEl = el;
        }
    }


int main() {
    std::thread t1(produce);
    std::thread t2(consume, onMessge);
    t1.join();
    t2.join();

}
