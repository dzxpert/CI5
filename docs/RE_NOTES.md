# CI5 reverse-engineering notes

Analysis used the running IDA MCP server at localhost:13337, with `CI5.runtime.snapshot.exe.i64`, image base **0x360000**. Runtime machine code for the stat setter and player finder matched the snapshot. All code references below are snapshot VAs; implementation constants are RVAs.

## Confirmed layout

| Object | Offset | Meaning |
|---|---:|---|
| PlayerInfo | 0x90 | Player ID (opaque 32-bit ID, not a small slot index) |
| PlayerInfo | 0xB0 | PlayerObject pointer |
| PlayerInfo | 0xB4 | Self pointer |
| PlayerInfo | 0xD8 | Encoded score |
| PlayerInfo | 0x1E8 | Encoded lives |
| PlayerInfo | 0x2F8 | Encoded food |
| PlayerInfo | 0x408 | Encoded missiles |
| PlayerInfo | 0x518 | Encoded next-extra-life score threshold, not medals |
| PlayerInfo | 0x738 | Damage-related HUD accumulator, not food |
| PlayerInfo | 0x848–0x850 | Coordinates, not score/lives/missiles |
| PlayerObject | 0x2AC | PlayerInfo owner pointer |
| PlayerObject | 0x2C8 | Weapon pointer |
| PlayerObject | 0x2D8 | Encoded remaining firing energy (1,000,000 = full) |
| PlayerObject | 0x4F8 | Encoded overheat latch |
| Weapon | 0x2C8 | Weapon type |
| Weapon | 0x2D0 | Double firing interval |
| Weapon | 0x2F0 | Float heat per shot, not bullet velocity |
| Weapon | 0x2F8 | Encoded power level |
| Profile System | `[module+0x5814E8]` + 3352 | Active profile slot index |
| Player Profile | `+ 102760` / `+ 102892` | XOR-encoded player keys bank (persistent store currency) |
| WaveController | `Role+0x2FC` -> `+ 0x94` -> `+ 0x398` / `+ 0x39C` | In-mission XOR-encoded keys collected |

`sub_365F20` constructs six encoded integers at 0xD8, 0x1E8, 0x2F8, 0x408, 0x518 and 0x628. HUD rendering in `sub_3DDC80` reads score/lives/missiles/food via the encoded getter. Missile firing in `sub_448D80` reads/decrements 0x408. Death handling in `sub_44D520` reads/decrements 0x1E8. `sub_36C3D0` handles food-to-missile conversion. `sub_369320` initializes extra-life thresholds at 0x518/0x628. `sub_366610` writes coordinate data to 0x848/0x84C/0x850.

`sub_3738D0` identifies the firing interval, heat cost, and remaining-energy field. `sub_44AA50` returns remaining energy / 1,000,000; the heat percentage is its complement. `sub_44AAB0` reads the overheat latch. The original +0x738 heat write was unrelated.

## Encoded integers

The container has 64 uint32 words followed by indices at 0x100, 0x104, 0x108 and an accumulated integrity-error word at 0x10C. Its value is `words[first] XOR words[second]`. Its check word is `check + words[first] * words[second]`, with unsigned 32-bit arithmetic.

- Constructor: `sub_4EEE80`.
- Getter: `sub_4EEFC0`, which also rotates the encoding and consumes the game's RNG.
- Setter: `sub_4EF000`, RVA **0x18F000**, x86 `__thiscall(object, int)`.

UI reads use validated snapshots without calling the mutating native getter. Writes invoke the native setter and validate readback. Partial/plain writes corrupt these containers.

## Role and thread context

`[module+0x5814F4]` is the engine context. Its +0x28 pointer is **UveNetwork**, not a permanent GameUniverse pointer. UveNetwork+0x0C holds a *temporarily active* role; it was null during external reads and EndScene. `sub_687010` confirms the UveNetwork constructor.

Role+0x1D0 is nonzero for the server role. Role+0x98 identifies the local player in the client role. The same local player's ID exists in two PlayerInfo objects (client and authoritative/server). Heap scanning cannot identify the authoritative object.

The mod hooks PlayerInfo update at `sub_366610`, RVA **0x6610**, preserving the x86 thiscall ABI and four stack words (two doubles). The client callback captures the local ID; only its matching server callback consumes queued actions. Direct stat edits are also staged to the matching client callback so both local copies agree. No role globals are fabricated or overwritten. The old discovery worker and first-heap-match logic were removed.

## Native commands

- Weapon-install command wrapper: `sub_44E510`, RVA **0xEE510**, `(PlayerObject*, int type, int power)`.
- Local weapon installer: `sub_44E580`. Calling this directly bypasses RPC context setup; the old implementation did that after corrupting the old weapon's type/power fields.
- Give-invulnerability command wrapper: `sub_44E910`, RVA **0xEE910**, `(PlayerObject*, double duration)`.
- Local shield implementation: `sub_44E970`, creates a real invulnerability entity. Its timer at +0x2D0 decrements in `sub_457B70`.

God mode refreshes a two-second native shield using simulation time. It no longer writes PlayerObject+0x2518 or forcibly changes the lifecycle state to 3. A paused game does not accumulate shield entities.

## Validation

- MSVC x86 Release build passed.
- Regression tests passed for all index positions, signed boundary values, integrity failures, invalid indices, distinct stat offsets, and inverted heat conversion.
- Runtime encoded snapshots decoded correctly with zero integrity errors.
- Actual game hooks captured distinct local-client and authoritative PlayerInfo objects.
- The live test incremented, verified, and restored lives, missiles, score, and food on the authoritative update before returning to gameplay; all four passed.
- The user independently confirmed that setting lives worked in-game.
- Fullscreen HAL/REF dummy-device creation failed during validation. A NULLREF fallback allowed the D3D9 and PlayerInfo hooks to initialize successfully.

Remaining limits: visual screenshot capture failed in this session; missile consumption, score awards, shield collision behavior and every weapon type were not individually exercised. Direct stat edits are not complete gameplay events and do not reproduce associated profile/award side effects. Teleport remains experimental.
