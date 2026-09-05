# C++ conventions

Use Chromium-style C++ in `src/godotium`: `.cc` implementation files, `.h` headers,
two-space indentation, `PascalCase` functions/types, `snake_case` variables,
`member_` fields and `kConstant` constants. Headers have include guards and must
include the types they need. Avoid `using namespace`; prefer scoped names or
specific using declarations. Keep UI text in translation resources.

Format with a recent clang-format using the repository's `.clang-format`:

```sh
find src/godotium -type f \( -name "*.cc" -o -name "*.h" \) -exec clang-format -i {} +
```

Godot virtual methods (`_ready`, `_draw`, etc.), binding hooks and the C ABI entry
point retain their required names. Godot owns scene-tree nodes; use `godot::Ref`
for reference-counted resources and follow Godot ownership rules. Do not wrap
engine-owned nodes in owning standard smart pointers.

Game behavior belongs in C++. `.tscn` and `.tres` files describe scenes and
resources; GDScript is currently used only by the external UI test harness.
After changes, build the game and run the distribution test described in README.
GitHub Actions is manual: do not start it merely because a commit changed.
