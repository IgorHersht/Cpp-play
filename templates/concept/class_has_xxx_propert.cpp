
template<typename Class>
constexpr static bool class_has_xxx_property_v = requires { Class::XXX; requires ( Class::XXX == true);  };
template<typename Class> concept class_has_xxx_property = class_has_xxx_property_v<Class>;
struct A{};
struct B {
    constexpr static bool XXX = false;
};

struct C{
    constexpr static bool XXX = true;
};
static_assert(!class_has_xxx_property_v<A>);
static_assert(!class_has_xxx_property_v<B>);
static_assert(class_has_xxx_property_v<C>);
int main() {

}
