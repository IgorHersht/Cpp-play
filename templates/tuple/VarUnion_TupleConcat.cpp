#include <variant>
#include <tuple>

//
template <class ...Args> struct VariantUnionHelper;

template <class ...Args1, class ...Args2>
struct VariantUnionHelper<std::variant<Args1...>, std::variant<Args2...>> {
    using type = std::variant<Args1..., Args2...>;
};

template <class Variant1, class Variant2>
using VariantUnion = typename VariantUnionHelper<Variant1, Variant2>::type;

// test
using Var1 = std::variant<int, long, short>;
using Var2 = std::variant<bool, char, double>;
using VarUnion = VariantUnion<Var1, Var2>;
static_assert(std::is_same_v<VarUnion, std::variant<int, long, short, bool, char, double>>);

using Tuple1 = std::tuple<int, long, short>;
using Tuple2 = std::tuple<bool, char, double>;
using TupleConcat = decltype(std::tuple_cat(std::declval<Tuple1>(), std::declval<Tuple2>()));
static_assert(std::is_same_v<TupleConcat, std::tuple<int, long, short, bool, char, double>>);



int main()
{

 

}


