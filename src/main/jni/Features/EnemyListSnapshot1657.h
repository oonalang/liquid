#pragma once
#include <array>
#include <cstdint>

namespace EnemyListSnapshot1657 {
constexpr int kMaxPawns = 128;
struct Snapshot {
    std::array<std::uintptr_t, kMaxPawns> pawns{};
    int count = 0;
};

template <class List, class Validator>
bool Read(List* source, Validator valid, Snapshot& snapshot) {
    snapshot = Snapshot{};
    if (!source || !valid(source)) return false;
    const int count = source->getSize();
    if (count < 0 || count > kMaxPawns) return false;
    if (count == 0) return true;
    auto* array = source->items;
    if (!array || !valid(array)) return false;
    const int capacity = array->getLength();
    if (capacity < count || capacity < 0 || capacity > 65535) return false;
    const auto* values = source->getItems();
    if (!values) return false;
    for (int i = 0; i < count; ++i)
        snapshot.pawns[i] = static_cast<std::uintptr_t>(values[i]);
    snapshot.count = count;
    return true;
}
}
