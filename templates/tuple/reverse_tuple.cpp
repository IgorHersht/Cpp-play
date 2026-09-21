#include <array>

template<class T> concept class_type = std::is_class_v<T>;

struct to_any_type {
    template<typename T> operator T() { return T{}; }
};

template<class_type T>
consteval std::size_t count_members(auto ...members) {
    if constexpr (!requires { T{ members... }; })
        return sizeof...(members) - 1;
    else
        return count_members<T>(members..., to_any_type{});
}

// test
void foo(double d) {
    int i = 1;
}

int main() {
    int i = to_any_type{};// i = 0
    foo(to_any_type{}); // foo called with double d = 0;
    struct C { int j{}; };
    struct St { int i{ 1 }; char ch{ 'c' }; std::array<int, 5> ar{ 25 }; C c; };
    static_assert(count_members<St>() == 4);
}
