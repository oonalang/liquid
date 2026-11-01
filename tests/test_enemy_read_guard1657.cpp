// Host test for the ESP pawn-read guard. Same contract the triggerbot has: a pawn is not
// read until it has been in the list for a while, and a flag byte that is not 0 or 1 is
// treated as a torn read of an object the game is already tearing down.
#include "Features/EnemyReadGuard1657.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace G = EnemyReadGuard1657;

static int failures = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
            ++failures;                                                   \
        }                                                                 \
    } while (0)

static void test_settle_window() {
    G::Reset();
    G::settleMs = 300;
    G::forgetMs = 1000;

    const std::uintptr_t pawn = 0x1000;
    CHECK(!G::Settled(pawn, 1000));   // first sighting: the round may still be building
    CHECK(!G::Settled(pawn, 1299));   // 299 ms in: still not safe to read
    CHECK(G::Settled(pawn, 1300));    // 300 ms in: settled
    CHECK(G::Settled(pawn, 1400));    // keeps appearing: stays settled

    // A single missed frame does not re-arm anything.
    G::Reset();
    CHECK(!G::Settled(pawn, 2000));
    CHECK(!G::Settled(pawn, 2100));
    CHECK(G::Settled(pawn, 2300));
    CHECK(G::Settled(pawn, 2400));

    // A long absence forgets the address, so a pawn that comes back - the same pointer
    // reused after a round flip - has to settle again before its bones are read.
    const std::uintptr_t other = 0x2000;
    CHECK(!G::Settled(other, 3500));        // prune pass: pawn last touched at 2400
    CHECK(!G::Settled(pawn, 3501));         // re-armed, not read this frame
    CHECK(!G::Settled(pawn, 3799));
    CHECK(G::Settled(pawn, 3801));

    // Nothing settles instantly once the window is disabled.
    G::settleMs = 0;
    const std::uintptr_t third = 0x3000;
    CHECK(G::Settled(third, 4000));
    G::settleMs = 300;
}

static void test_table_bounds() {
    G::Reset();
    G::settleMs = 0;
    // More pawns than slots: the table must not run off the end, and lookups keep working.
    for (int i = 0; i < 400; ++i) G::Settled(0x10000u + (unsigned)i * 0x10u, 1000);
    CHECK(G::entryCount <= static_cast<unsigned>(G::kSlots));
    CHECK(G::Settled(0x10000u + 0 * 0x10u, 1000));
    G::Reset();
    CHECK(G::entryCount == 0);
}

static void test_flag_bytes() {
    struct Fake {
        std::uint8_t alive;
        std::uint8_t bot;
        std::uint8_t rest[6];
    } f{1, 0, {}};
    const auto base = reinterpret_cast<std::uintptr_t>(&f);
    bool value = false;

    CHECK(G::ReadFlag(base, offsetof(Fake, alive), value));
    CHECK(value);
    CHECK(G::ReadFlag(base, offsetof(Fake, bot), value));
    CHECK(!value);

    f.alive = 0;
    CHECK(G::ReadFlag(base, offsetof(Fake, alive), value));
    CHECK(!value);

    // A destroyed or reused object does not hold 0/1 here.
    f.alive = 2;
    CHECK(!G::ReadFlag(base, offsetof(Fake, alive), value));
    f.alive = 0xFF;
    CHECK(!G::ReadFlag(base, offsetof(Fake, alive), value));
    CHECK(!value);  // untouched on rejection
}

int main() {
    test_settle_window();
    test_table_bounds();
    test_flag_bytes();
    if (failures) {
        std::printf("%d failure(s)\n", failures);
        return 1;
    }
    std::printf("enemy read guard: ok\n");
    return 0;
}
