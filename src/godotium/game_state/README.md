# GameState library

An MIT-licensed C++17 example of reusable game logic with no Godot, filesystem,
or platform dependency. GN compiles `game_state.cc` into `libgame_state.a`.
SCons links that archive into the GDExtension; it does not recompile the source.

```cpp
#include "godotium/game_state/game_state.h"

godotium::GameState game;
game.Advance(0.25);
auto snapshot = game;  // An independent value, ready for persistence.
```

A new state starts at zero. `Advance(delta_seconds)` accumulates finite,
nonnegative time steps. Invalid states, negative/nonfinite deltas and steps
beyond `kMaxElapsedSeconds` return `false` without changing the state. `IsValid()`
also validates loaded snapshots. Each instance is independent; there is no
singleton or implicit clock. The caller chooses when to advance, so pausing and
deterministic tests use the same API.

`GameScene` supplies frame deltas, pauses processing, and formats the state for
display. `SaveRepository` serializes the value and checks loaded data. Those
Godot adapters remain outside this library. Add game rules and persistent fields
here as the template grows; extend the repository's versioned format when needed.
Settings are ordinary application code in `godotium/settings/game_settings.cc`.

## Build

From `src`, with the project tools on PATH:

```sh
gn gen out/Default
ninja -C out/Default game_state
```

This target requires only the native C++ toolchain, not Godot or godot-cpp.

`ninja -C out/Default godotium` automatically builds the correct library archive
for the game. A cross export also builds a host archive for the editor extension.

| Build | Archive relative to the build directory |
| --- | --- |
| Host | `obj/godotium/game_state/libgame_state.a` |
| Windows cross export from Mac | `win/obj/godotium/game_state/libgame_state.a` |
| Linux cross export from Mac | `linux/obj/godotium/game_state/libgame_state.a` |

The library uses PIC on macOS/Linux, the same LLVM-MinGW runtime family as
Godot on Windows, and the existing Docker toolchain for Linux cross exports.
Its target is `//godotium/game_state:game_state`. `godotium/BUILD.gn` supplies this
label and its public header to `game_cpp_library`; keep its implementation out
of `game_sources.json` to avoid compiling it twice.

## Moving to a separate repository

This value-based API can be supplied by a separate source repository. Pin the
source revision and checksum, provide any private-repository credentials through
the build environment, and keep build integration outside the downloaded tree.

The interface is source-level C++, not a stable binary ABI. Build the library
with the same architecture, compiler ABI and runtime as its consumer. A private
repository does not hide the compiled logic in a distributed game; existing MIT
versions retain their license.
