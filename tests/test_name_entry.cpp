#include <cassert>
#include <cstring>

#include "UI/NameEntry.h"
#include "UI/NamedConfigStorage.h"

int main() {
    using NameEntry::Append;
    using NameEntry::Backspace;
    using NameEntry::Savable;

    char name[64]{};
    assert(!Append(name, sizeof(name), ' '));          // no leading space
    assert(Append(name, sizeof(name), 'A'));
    assert(Append(name, sizeof(name), ' '));
    assert(Append(name, sizeof(name), 'b'));
    assert(Append(name, sizeof(name), '1'));
    assert(std::strcmp(name, "A b1") == 0);
    assert(Savable(name));
    assert(NamedConfigStorage::ValidName(name));

    // Only characters ValidName accepts can be typed.
    for (const char rejected : {'/', '\\', ':', '*', '?', '"', '<', '>', '|', '.', '+', '#'})
        assert(!Append(name, sizeof(name), rejected));
    assert(std::strcmp(name, "A b1") == 0);

    assert(Backspace(name));
    assert(std::strcmp(name, "A b") == 0);
    assert(Backspace(name) && std::strcmp(name, "A ") == 0);

    // Every accepted key produces a name the storage layer accepts.
    char every[64]{};
    for (const char* p = "abzAZ09_ -"; *p; ++p) Append(every, sizeof(every), *p);
    assert(NamedConfigStorage::ValidName(every));

    // The length cap is the storage limit, not the buffer size.
    char full[64]{};
    for (int i = 0; i < 40; ++i) assert(Append(full, sizeof(full), 'x'));
    assert(std::strlen(full) == 40);
    assert(!Append(full, sizeof(full), 'y'));
    assert(Savable(full) && NamedConfigStorage::ValidName(full));
    assert(Backspace(full) && std::strlen(full) == 39);

    // A name that would not fit is refused whole, never truncated into the buffer.
    char tiny[3] = "ab";
    assert(!Append(tiny, sizeof(tiny), 'c'));
    assert(std::strcmp(tiny, "ab") == 0);

    // Empty is not savable, and backspace on empty does nothing.
    char empty[8]{};
    assert(!Savable(empty));
    assert(!Backspace(empty));
    assert(Append(empty, sizeof(empty), '-'));
    assert(std::strcmp(empty, "-") == 0);

    // Trailing space is not savable, but the name stays editable.
    char trailing[8]{};
    Append(trailing, sizeof(trailing), 'a');
    Append(trailing, sizeof(trailing), ' ');
    assert(!Savable(trailing));
    assert(Backspace(trailing) && Savable(trailing));
}
