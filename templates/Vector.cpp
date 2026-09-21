#include <type_traits>

template<int... I> struct Vector {};

// element-wise multiply two Vectors
template<int... X, int... Y>
constexpr auto elem_mul(Vector<X...>, Vector<Y...>) {
    static_assert(sizeof...(X) == sizeof...(Y), "size mismatch");
    return Vector<(X * Y)...>{};
}

// multiply N Vectors (N >= 1)
template<typename V>
constexpr auto multiply(V v) { return v; }

template<typename V1, typename V2, typename... Vs>
constexpr auto multiply(V1 a, V2 b, Vs... vs) {
    return multiply(elem_mul(a, b), vs...);
}

template<typename... V>
using zip = decltype(multiply(V{}...));

// test
static_assert(std::is_same_v < zip<Vector<1, 2, 3>, Vector<4, 5, 6>, Vector<2, 3, 4>>, decltype(Vector<8, 30, 72>{}) > );

int main() {}