#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>
#include "../src/main/jni/Features/EnemyListSnapshot1657.h"

struct TestArray {
    int capacity = 0;
    std::vector<std::uintptr_t> data;
    int getLength() const { return capacity; }
};
struct TestList {
    TestArray* items = nullptr;
    int size = 0;
    int getSize() const { return size; }
    const std::uintptr_t* getItems() const { return items->data.data(); }
};
int main() {
    using namespace EnemyListSnapshot1657;
    auto valid = [](const void* p) { return p != nullptr; };
    Snapshot snapshot;
    assert(!Read(static_cast<TestList*>(nullptr), valid, snapshot));
    TestArray array{3, {12, 34, 56}};
    TestList list{&array, 3};
    assert(Read(&list, valid, snapshot));
    assert(snapshot.count == 3 && snapshot.pawns[0] == 12 && snapshot.pawns[2] == 56);
    list.size = 4;
    assert(!Read(&list, valid, snapshot) && snapshot.count == 0);
    list.size = -1;
    assert(!Read(&list, valid, snapshot));
    list.size = 129;
    assert(!Read(&list, valid, snapshot));
    list.size = 3;
    list.items = nullptr;
    assert(!Read(&list, valid, snapshot));
    list.items = &array;
    assert(Read(&list, [](const void* p) { return p != nullptr && p != (const void*)0x1; }, snapshot));
    Snapshot cleared = snapshot;
    assert(cleared.count == 3);
    array.capacity = 2;
    assert(!Read(&list, valid, snapshot));
    assert(snapshot.count == 0 && snapshot.pawns[0] == 0);
    array.capacity = 3;
    list.size = 0;
    assert(Read(&list, valid, snapshot) && snapshot.count == 0);
    std::cout << "PASS: bounded enemy list snapshots and malformed transitions\n";
}
