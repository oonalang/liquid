#pragma once
#include <cstddef>
#include <cstring>

// The overlay is drawn over the game's own window and owns no Android view, so the
// system keyboard is never attached to a text field inside it - which is why the license
// screen offers a "Paste key" button. The config panel therefore draws its own keyboard,
// and these are the edits that keyboard applies to the name buffer. Kept apart from the
// drawing code so the rules can be checked without a renderer.
namespace NameEntry {

// NamedConfigStorage::ValidName rejects anything longer than this.
inline constexpr std::size_t kMaxLength = 40;

// Exactly the characters a profile name may contain, so a name typed with the panel's
// keyboard can never fail validation for what it is made of.
inline bool Accepted(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == ' ' || c == '_' || c == '-';
}

inline bool Append(char* buffer, std::size_t size, char c) {
    if (!buffer || size < 2 || !Accepted(c)) return false;
    const std::size_t used = std::strlen(buffer);
    // A leading space is what ValidName rejects, so it is refused up front instead of
    // leaving a name that cannot be saved.
    if (c == ' ' && used == 0) return false;
    if (used >= kMaxLength || used + 1 >= size) return false;
    buffer[used] = c;
    buffer[used + 1] = '\0';
    return true;
}

inline bool Backspace(char* buffer) {
    if (!buffer) return false;
    const std::size_t used = std::strlen(buffer);
    if (used == 0) return false;
    buffer[used - 1] = '\0';
    return true;
}

inline bool Savable(const char* buffer) {
    if (!buffer) return false;
    const std::size_t length = std::strlen(buffer);
    return length > 0 && length <= kMaxLength &&
           buffer[0] != ' ' && buffer[length - 1] != ' ';
}

} // namespace NameEntry
