#pragma once
#include <algorithm>
#include <cstddef>
#include <vector>

// The instance lists are filled from game constructors that keep running for the whole
// session as content loads. They have to stay bounded - an unbounded list made every
// rebuild slower the longer a session ran - but a ceiling must never act as a content
// limit. 1.6.57 has far more weapon configs than the old cap, so refusing new entries
// silently hid every skin the game constructed after it: the panel only ever showed the
// first slice of the content. The newest entries are the ones being used, so the oldest
// are dropped instead of the newest being turned away.
namespace CaptureRing {

inline constexpr std::size_t kEvictBatch = 1024;

inline void Add(std::vector<void*>& list, std::size_t limit, void* value) {
    if (!value || limit == 0) return;
    if (list.size() >= limit) {
        const std::size_t drop = std::min(kEvictBatch, list.size());
        list.erase(list.begin(), list.begin() + static_cast<std::ptrdiff_t>(drop));
    }
    list.push_back(value);
}

} // namespace CaptureRing
