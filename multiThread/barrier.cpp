//********************************************************
// The following code example is taken from the book
//  C++20 - The Complete Guide
//  by Nicolai M. Josuttis (www.josuttis.com)
//  http://www.cppstd20.com
//
// The code is licensed under a
//  Creative Commons Attribution 4.0 International License
//  http://creativecommons.org/licenses/by/4.0/
//********************************************************


#include <iostream>
#include <format>
#include <vector>
#include <thread>
#include <cmath>
#include <barrier>

int main()
{
  // initialize and print a collection of floating-point values:
  std::vector values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

  // define a lambda function that prints all values
  // - NOTE: has to be noexcept to be used as barrier callback
  auto printValues = [&values] () noexcept{
                       for (auto val : values) {
                         std::cout << std::format(" {:<7.5}", val);
                       }
                       std::cout << '\n';
                     };
  // print initial values:
  printValues();

  // initialize a barrier that prints the values when all threads have done their computations:
  std::barrier allDone{int(values.size()),  // initial value of the counter
                       printValues};        // callback to call whenever the counter is 0

  // initialize a thread for each value to compute its square root in a loop:
  // i = 0
 // first thread reaches the allDone.arrive_and_wait() and wait
 // next thread reaches the allDone.arrive_and_wait() and wait
 //..
 // last thread reaches the allDone.arrive_and_wait() and wait
 // counter reset
 // i = 1
 // first thread reaches the allDone.arrive_and_wait() and wait
 // next thread reaches the allDone.arrive_and_wait() and wait
 //..
 // 
  std::vector<std::jthread> threads;
  for (std::size_t idx = 0; idx < values.size(); ++idx) {
    threads.push_back(std::jthread{[idx, &values, &allDone] {
                                     // repeatedly:
                                     for (int i = 0; i < 5; ++i) {
                                       // compute square root:
                                       values[idx] = std::sqrt(values[idx]);
                                       // and synchronize with other threads to print values:
                                        std::cout << "thread=" << idx << " Value at=" << i << '\n';
                                       allDone.arrive_and_wait();
                                     }
                                    int i = 0;
                                   }});
  }
  //...
}

/*
 *

 1       2       3       4       5       6       7       8
thread=0 Value at=0
thread=3 Value at=0
thread=7 Value at=0
thread=2 Value at=0
thread=4 Value at=0
thread=5 Value at=0
thread=1 Value at=0
thread=6 Value at=0
 1       1.4142  1.7321  2       2.2361  2.4495  2.6458  2.8284
thread=5 Value at=1
thread=2 Value at=1
thread=0 Value at=1
thread=3 Value at=1
thread=7 Value at=1
thread=4 Value at=1
thread=6 Value at=1
thread=1 Value at=1
 1       1.1892  1.3161  1.4142  1.4953  1.5651  1.6266  1.6818
thread=1 Value at=2
thread=5 Value at=2
thread=4 Value at=2
thread=0 Value at=2
thread=3 Value at=2
thread=7 Value at=2
thread=2 Value at=2
thread=6 Value at=2
 1       1.0905  1.1472  1.1892  1.2228  1.251   1.2754  1.2968
thread=6 Value at=3
thread=1 Value at=3
thread=5 Value at=3
thread=2 Value at=3
thread=0 Value at=3
thread=3 Value at=3
thread=7 Value at=3
thread=4 Value at=3
 1       1.0443  1.0711  1.0905  1.1058  1.1185  1.1293  1.1388
thread=1 Value at=4
thread=6 Value at=4
thread=5 Value at=4
thread=2 Value at=4
thread=0 Value at=4
thread=3 Value at=4
thread=4 Value at=4
thread=7 Value at=4
 1       1.0219  1.0349  1.0443  1.0516  1.0576  1.0627  1.0671
*/

