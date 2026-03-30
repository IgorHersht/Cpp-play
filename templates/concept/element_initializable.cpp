#include <vector>
template <typename T> concept  element_initializable   = requires(T){ T(std::size_t{}, typename T::value_type{}); };

template<element_initializable  Container >  auto init ( std::size_t size, const typename Container::value_type& value) {
    Container container(size, value);
    return container;
};
// test

#include <assert.h>
int main() {
    using Vector = std::vector<int>;
    Vector v{2, 2, 2, 2, 2};
    Vector v1 = init <Vector>(5,2);
    assert(v1 == v);
}
