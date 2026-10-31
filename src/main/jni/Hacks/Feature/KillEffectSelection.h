#pragma once

namespace OblivionSkinHax {

inline int SelectKillEffectId(int configuredId, int matchingItemId) {
    if (configuredId > 0) return configuredId;
    return matchingItemId > 0 ? matchingItemId : 0;
}

// Full resolution order for a skin's kill effect. A mythic skin usually leaves the
// kill effect on the base weapon's extra config rather than on the skin entry, so
// reading only the skin left it at zero: no kill visual and no kill sound, while the
// firing effect (a separate field) still worked. The compatibility item id is the
// last resort, and it is only used when neither config carries an authored id.
inline int ResolveKillEffectId(int authoredId, int baseAuthoredId, int matchingItemId) {
    if (authoredId > 0) return authoredId;
    if (baseAuthoredId > 0) return baseAuthoredId;
    return matchingItemId > 0 ? matchingItemId : 0;
}

}
