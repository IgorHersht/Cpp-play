#include <type_traits>
#include <tuple>
#include <cstddef>
template<typename From, typename To> constexpr static bool is_same_base_type_v= std::is_same_v<std::decay_t<From>, std::decay_t<To>>;

//test
int i; int& ri = i; const int& cri = i; int&& rri = std::move(i); const int& crri = std::move(i);
static_assert(is_same_base_type_v<decltype(i), decltype(ri)>);
static_assert(is_same_base_type_v<decltype(ri), decltype(cri)>);
static_assert(is_same_base_type_v<decltype(cri), decltype(rri)>);
static_assert(is_same_base_type_v<decltype(rri), decltype(crri)>);

int main() {}
