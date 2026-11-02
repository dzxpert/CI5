# CI5 Galactic Commander

An x86 internal mod for the analyzed Steam build of Chicken Invaders 5, with a Dear ImGui / Direct3D 9 menu.

## Build and load

1. Run `build.bat` (MSVC x86, CMake, and Ninja or the Visual Studio generator are required).
2. Open CI5 and run `inject.bat`. The script works regardless of the launcher's current directory and keeps errors visible.
3. Press **INSERT** for the menu. Press **END** to unload before rebuilding or reinjecting.

The normal outputs are `build/CI5Mod.dll` and `build/CI5Injector.exe` (or `build/Release/` with a Visual Studio generator). The injector rejects duplicate loading and reports success only after the DLL confirms that its hooks initialized. Initialization and action results are logged to `CI5Mod.log` beside the DLL.

## Controls

- Read and set lives, missiles, score, and food; add preset amounts.
- Maintain a configurable minimum number of lives/missiles.
- Reset weapon heat and keep firing energy full, including clearing the overheat latch.
- Equip weapons through the game's command wrapper; inspect decoded weapon power.
- Refresh the game's native two-second spawn shield. Disabling the toggle lets the existing shield expire in simulation time.
- Override the weapon firing interval.

Changes are queued for the local authoritative simulation update. The menu reports queued, applied, expired, and failed changes. Start/resume a mission if no player update is available. This workflow targets local single-player/localhost play; it cannot alter a remote server's authoritative player.

Direct stat edits update the encoded value in both local simulation copies. They do not simulate collecting food, award score bonuses, or immediately grant extra lives when a score threshold is crossed. Normal subsequent gameplay still runs the game's normal award logic. Setting lives does not resurrect an already eliminated player.

Projectile speed control is unavailable: the old alleged speed field was actually weapon heat per shot. The old alleged medals field is an extra-life score threshold and is now labeled accordingly. Teleport controls remain experimental because player input/network prediction can overwrite position changes.

## Implementation and validation

The original mod treated coordinates and encoded fields as plain stat integers and selected the first heap match. The fixed implementation captures the actual local player during its update, executes writes with the authoritative role active, and uses CI5's native encoded-integer setter. Native weapon/shield commands also run in that context.

See [reverse-engineering notes](docs/RE_NOTES.md) for verified offsets and evidence. Addresses are build-specific; initialization checks selected instruction bytes and refuses an incompatible build.

Regression tests can be enabled with `-DCI5MOD_BUILD_TESTS=ON`, then run with `ctest --test-dir build --output-on-failure`. A separate `-DCI5MOD_LIVE_TEST=ON` validation build exposes a test-only entry point that queues increment/readback/restore checks in the authoritative update. It is disabled in normal builds.
