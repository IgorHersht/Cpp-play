#include <vector>
template <typename T> concept  resizable_container  = requires(T t) { t.resize(std::size_t{}, typename T::value_type{}); };

template<resizable_container Container >  auto init( std::size_t size, const typename Container::value_type& value) {
    Container container{};
    container.resize(size, value);
    return container;
};
// test

#include <assert.h>
int main() {
    using Vector = std::vector<int>;
    Vector v{2, 2, 2, 2, 2};
    Vector v1 = init<Vector>(5,2);
    assert(v1 == v);
}
