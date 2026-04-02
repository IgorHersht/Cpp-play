#include <type_traits>
#include <tuple>
#include <cstddef>


//https://www.youtube.com/watch?v=X_w_pcPs2Fk
//https://github.com/CppCon/CppCon2025
//"	How to Tame Packs, std::tuple, and the Wily std::integer_sequence by Andrei Alexandrescu


template <size_t n, typename F, size_t... i>
constexpr auto unroll(F&& f, std::index_sequence<i...> = std::index_sequence<>()) {
    if constexpr (sizeof...(i) != n) {
        return unroll<n>(std::forward<F>(f), std::make_index_sequence<n>());
    } else {
        using result_t = decltype(f(std::integral_constant<size_t, 0>()));
        if constexpr (std::is_void_v<result_t>) {
            return (f(std::integral_constant<size_t, i>()), ...);
        } else {
            return (f(std::integral_constant<size_t, i>()) && ...);
        }
    }
}

template <typename Tuple, typename F>
constexpr void each_in_tuple(Tuple&& t, F&& f) {
    constexpr size_t n = std::tuple_size_v<::std::remove_reference_t<Tuple>>;
    return unroll<n>([&](auto i) {
      if constexpr (requires{f(i, std::get<i>(::std::forward<Tuple>(t)));}) {
        return f(i, std::get<i>(::std::forward<Tuple>(t)));
      } else {
        return f(std::get<i>(std::forward<Tuple>(t)));
      }
    });
}

template <typename Tuple, typename F>
constexpr void for_each3(Tuple&& t, F&& f) {
    constexpr size_t n = std::tuple_size_v<::std::remove_reference_t<Tuple>>;
    return unroll<n>([&](auto i) {
        return f(std::get<i>(std::forward<Tuple>(t)));
    });
}

// test


auto increment = []( auto& t) {
    ++t ;
};


int main() {
    std::tuple tp{ 10, 5.2, 3l, (short) 2};
    for_each3(tp,increment );
    int i = 0;
}
