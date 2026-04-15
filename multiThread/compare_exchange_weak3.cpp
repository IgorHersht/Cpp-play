#include <atomic>
#include <thread>

template <typename T> struct StackT{
    struct Node {
        T data{};
        Node* next{};
        Node(const T& data) : data(data), next(nullptr) {}
    };

    std::atomic< Node*> head{};

void push(const T& data) {
    Node* newNode = new Node(data);
    newNode->next = head.load(std::memory_order_relaxed);

    while (!head.compare_exchange_weak(newNode->next, newNode, std::memory_order_release, std::memory_order_relaxed));
}
};

/*
bool compare_exchange_strong (T& expected, T desired)
{
if (this->load() == expected) {
this->store(desired);
return true;
}
else {
expected = this->load();
return false;
}
}
*/

// test
#include <iostream>

int main() {
    using Stack = StackT<int>;
    Stack stack;
    {
        std::jthread thread0([&stack]() {
            for (int i=0;i<100;i++) {
                if (i%2 == 0) {
                 stack.push(i);
                } else {
                    std::this_thread::yield();
                }

            }
        });
        std::jthread thread1([&stack]() {
           for (int i= 1;i<101;i++) {
               if (i%2 != 0) {
                stack.push(i);
               } else {
                   std::this_thread::yield();
               }
           }
       });
    }

    Stack::Node* node = stack.head.load();
    while (node) {
        std::cout << node->data << std::endl;
        node = node->next;
    }



}
