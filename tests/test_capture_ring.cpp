#include <cassert>
#include <cstddef>
#include <vector>

#include "Hacks/Feature/CaptureRing.h"

int main() {
    using CaptureRing::Add;

    std::vector<void*> list;
    constexpr std::size_t limit = 16;

    // Under the limit nothing is dropped, and order is preserved.
    for (std::size_t i = 0; i < limit; ++i) Add(list, limit, reinterpret_cast<void*>(i + 1));
    assert(list.size() == limit);
    assert(list.front() == reinterpret_cast<void*>(1));
    assert(list.back() == reinterpret_cast<void*>(limit));

    // Past the limit the oldest are evicted, not the newest refused: the most recent
    // capture is always present, which is what the skin list needs.
    Add(list, limit, reinterpret_cast<void*>(999));
    assert(list.size() <= limit);
    assert(list.back() == reinterpret_cast<void*>(999));
    assert(list.front() != reinterpret_cast<void*>(1)); // the oldest quarter is gone

    // The ceiling holds no matter how much more arrives.
    for (std::size_t i = 0; i < 5000; ++i) {
        Add(list, limit, reinterpret_cast<void*>(10000 + i));
        assert(list.size() <= limit);
    }
    assert(list.back() == reinterpret_cast<void*>(14999));

    // A null instance is never stored, and a zero limit stores nothing.
    const std::size_t before = list.size();
    Add(list, limit, nullptr);
    assert(list.size() == before);
    std::vector<void*> none;
    Add(none, 0, reinterpret_cast<void*>(1));
    assert(none.empty());

    // A batch larger than the limit still leaves a sane list.
    std::vector<void*> small;
    for (std::size_t i = 0; i < 2048; ++i) Add(small, 4, reinterpret_cast<void*>(i + 1));
    assert(small.size() <= 4);
    assert(small.back() == reinterpret_cast<void*>(2048));
}
