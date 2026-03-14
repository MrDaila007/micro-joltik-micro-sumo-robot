# Changelog

## Stable v1.0 (from test firmware)

### Bug Fixes
- **`startRoutine()` line 110:** `if(searchDir = RIGHT)` used assignment `=` instead of comparison `==`, causing the robot to always turn right at startup regardless of the random direction chosen earlier.
- **`attack()` lines 248, 258:** `searchDir == LEFT` and `searchDir == RIGHT` used comparison `==` instead of assignment `=`, so `searchDir` was never updated during attack. The robot would always search in its initial direction after losing an opponent.
- **`attack()` line 237:** The all-three-sensors branch (`OPPONENT_FC && FR && FL`) was unreachable because the single `OPPONENT_FC` check came first. Reordered so all-three-sensors triggers MAX_SPEED before the single-center check.
- **`loop()` bracket scoping:** Fixed the `#ifdef ENABLE_LINE_SEN` conditional bracket structure so the `else` block and its closing brace are properly paired.
- **`EEPROM.begin()` (line 310):** Removed stray space in `EEPROM. begin(512)`.
- **Variable shadowing in `checkIRReceive()`:** Renamed local `int address` to `int eepromAddr` to avoid shadowing the function parameter `unsigned char address`.

### Typo Fixes
- `ATACK_SPEED` → `ATTACK_SPEED`
- `SVAP_MOTORS` → `SWAP_MOTORS`
- `chekIReceive()` → `checkIRReceive()`
- Debug log `"Search_Right"` printed for LEFT direction → fixed to `"Search_Left"`

### Removed
- Commented-out dead attack code (lines 266-298) removed for clarity.
