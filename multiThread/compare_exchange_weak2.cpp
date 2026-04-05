#include <atomic>
#include <iostream>
#include <ostream>
#include <thread>

template<typename T>
struct lock_free_stack {
    struct node
    {
        T data;
        node* next{nullptr};
        node(T const& data_):
            data(data_)
        {}
    };
    std::atomic<node*> head{nullptr};
public:
    void push(T const& data)
    {
        node* const new_node=new node(data);
        new_node->next=head.load();
        while(!head.compare_exchange_weak(new_node->next,new_node));
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

//test


int main() {
    using Stack = lock_free_stack<int>;
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

    Stack::node* node = stack.head.load();
    while (node) {
        std::cout << node->data << std::endl;
        node = node->next;
    }



}
