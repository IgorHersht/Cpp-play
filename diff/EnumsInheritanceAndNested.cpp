#include <cstdint>

struct Colors
{
    using Type = std::uint8_t;
    struct Basic {
        enum : Type
        {
            Red,
            Green,
        };
        static constexpr Type last{ Green };
    };

    struct Extended : Basic {
        enum : Type
        {
            Blue = Basic::last + 1
        };
        static constexpr Type last{ Blue };
    };

    struct Other
    {
        enum : Type
        {
            Orange = Extended::last + 1,
            Purple,
            Yellow
        };
        static constexpr Type last{ Yellow };
    };
    static constexpr Type size{ Other::last + 1 };
};

static_assert(Colors::Basic::Red == 0);
static_assert(Colors::Basic::Green == 1);
static_assert(Colors::Extended::Blue == 2);
static_assert(Colors::Extended::Green == 1);// in base
static_assert(Colors::Other::Orange == 3);
static_assert(Colors::Other::Purple == 4);
static_assert(Colors::Other::Yellow == 5);
static_assert(Colors::size == 6);
int main() {
}
