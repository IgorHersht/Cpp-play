
//
// CODE FOR EXPANDING TUPLES INTO A CALLABLE
//https://genix.wordpress.com/2015/02/14/tuples-expanding-into-function-parameters/
 
#if _MSC_VER == 1900 // hack for VS2015 CTP 5 broken decltype(auto) deduction in tuple_into_callable_n
#define TUPLE_FWD_RETURN(x) std::conditional_t< std::is_rvalue_reference<decltype(x)>::value, std::remove_reference_t<decltype(x)>, decltype(x)>(x)
#else
#define TUPLE_FWD_RETURN(x) x
#endif
 
// support for expanding tuples into an overloaded function call's arguments
#define wrap_overload(func) [](auto&&... ps){ return func( std::forward<decltype(ps)>(ps)... ); }
 
namespace detail
{
    //
    // base case for building up arguments for the function call
    //
    template< typename CALLABLE, typename TUPLE, int INDEX >
    struct tuple_into_callable_n
    {
        template< typename... Vs >
        static auto apply(CALLABLE f, TUPLE t, Vs&&... args) -> decltype(auto)
        {
            return tuple_into_callable_n<CALLABLE, TUPLE, INDEX - 1>::apply(
                f, 
                std::forward<decltype(t)>(t), 
                std::get<INDEX - 1>(std::forward<decltype(t)>(t)),
                std::forward<Vs>(args)...
            );
        }
    };
 
    //
    // terminal case - do the actual function call
    //
    template< typename CALLABLE, typename TUPLE >
    struct tuple_into_callable_n< CALLABLE, TUPLE, 0 >
    {
        template< typename... Vs >
        static auto apply(CALLABLE f, TUPLE t, Vs&&... args) -> decltype(auto)
        {
            return TUPLE_FWD_RETURN(f(std::forward<Vs>(args)...));
        };
    };
}
 
template< typename FUNC, typename TUPLE >
auto tuple_into_callable(FUNC f, TUPLE&& t) -> decltype(auto)
{
    return
        detail::tuple_into_callable_n< 
            FUNC, 
            decltype(t), 
            std::tuple_size< std::remove_reference_t<TUPLE> >::value
        >::apply(f, std::forward<decltype(t)>(t) );
}

#include <iostream>
#include <tuple>
#include <string>
#include <utility>
#include <type_traits>
 
// TODO: INSERT THE TUPLE EXPANSION CODE LISTED ABOVE HERE
 
//
// TESTING TYPES & FUNCTIONS
//
using namespace std;
 
class MoveOnly
{
public:
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&& m) {};
 
    static MoveOnly Create()
    {
        return MoveOnly();
    }
 
private:
    MoveOnly() {}
};
 
 
string two_params(int i, int j)
{
    return "two_params int overload called.";
}
 
string two_params(float const& i, float const& j)
{
    return "two_params float const& overload called.";
}
 
string two_params(float&& i, float&& j)
{
    return "two_params float&& overload called.";
}
 
string move_only_receiver(MoveOnly&& m)
{
    return "move_only_receiver called.";
}
 
string no_params()
{
    return "no_params called.";
}
 
int main() {
 
    // lvalue tuple
    auto t = make_tuple(1, 2);
    cout << tuple_into_callable(wrap_overload(two_params), t) << endl;
 
    // rvalue tuple
    cout << tuple_into_callable(wrap_overload(two_params), make_tuple(1.0f, 2.0f)) << endl;
 
    // const tuple
    auto const ct = make_tuple(1.0f, 2.0f);
    cout << tuple_into_callable(wrap_overload(two_params), ct) << endl;
 
    // empty tuple -> empty function
    auto et = make_tuple();
    cout << tuple_into_callable(no_params, et) << endl;
 
    // tuple with move-only type
    auto move_only = MoveOnly::Create();
    auto mt = make_tuple(move(move_only));
    cout << tuple_into_callable(move_only_receiver, move(mt)) << endl; // note: tuple must be move'd into the callable so MoveOnly can be treated as &&
 
  return 0;
}
