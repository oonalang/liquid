#include <cassert>
#include <mutex>
#include <unordered_map>

std::mutex effectMutex;
std::unordered_map<int, int> selectedEffects;
int calls = 0;

int Resolve(int id, int (*fallback)());
int original() {
    ++calls;
    selectedEffects[20] = 77;
    return Resolve(20, nullptr);
}
int Resolve(int id, int (*fallback)()) {
    {
        std::lock_guard<std::mutex> lock(effectMutex);
        auto found = selectedEffects.find(id);
        if (found != selectedEffects.end() && found->second > 0) return found->second;
    }
    return fallback ? fallback() : 0;
}
int main() {
    assert(Resolve(10, original) == 77);
    assert(calls == 1);
    assert(Resolve(20, original) == 77);
    assert(calls == 1);
}
