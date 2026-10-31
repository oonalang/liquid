# LIQUID r15 Release Update

- Triggerbot uses the visual firing circle radius exactly; no projected-body-width expansion.
- Alternate visible samples must independently pass the same radius check.
- Removed the firing diagnostics panel and extra unfiltered diagnostic physics casts.
- Reduced movement, triggerbot, and dot descriptions to 1–5 words.
- Added No Hit Flinch in Weapon Handling, saved and restored with named configs.
- Uses 1.6.57 metadata for local-player-only hit-punch hook paths.
- Host tests: 24/24 legacy GCC/Clang regression runs passed; four new runs passed.
- New strict-zone and hit-flinch tests passed under ASan/UBSan.
- Android compilation and in-game behavior are not verified.
- No report-suppression hooks were added.
- IL2CPP dumps and compiled game libraries excluded.
