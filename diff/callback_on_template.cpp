#include <string>
#include <map>
#include <iostream>
#include <memory>


template <int i> bool c1(int j) {
    return i > j;
}

template < typename R, class Ft, typename... Args> R foo(Ft&& ft, Args&&... args) {
    return std::forward<Ft>(ft)(std::forward<Args>(args)...);
}

int main() {

    auto r3 = foo<bool>(c1<5>, 5);
    int i = 1;

    return 0;
}
