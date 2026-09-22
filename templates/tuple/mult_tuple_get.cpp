#include <string>
#include <tuple>
#include <type_traits>

template<class> inline constexpr bool always_false_v = false;

template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};


using Tuple1 = std::tuple<char, uint32_t>;

using Tuple2 = std::tuple<double, std::string>;

 Tuple1 t1('a', 10);
 Tuple2 t2(2.2, std::string("XXX") );

template <typename T> T& get() {
    if constexpr (has_type< T,Tuple1>::value) {
        return std::get<T>(t1);
    } else if constexpr (has_type< T,Tuple2>::value) {
        return std::get<T>(t2);
    } else  {
        static_assert( always_false_v<T>, "bad type");
    }
}

int main()
{
   auto v1 = get<uint32_t>();
   auto v2 = get<double>();
    auto v3 = get<std::string>();
   //auto v3 = get<uint64_t>();
    int i =1;
}
