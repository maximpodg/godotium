#include "godotium/music_player/register_types.h"

#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/godot.hpp"
#include "godotium/scenes/register_types.h"
#include "godotium/settings/register_types.h"

namespace {
void InitializeGodotium(godot::ModuleInitializationLevel level) {
  if (level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
    godotium::RegisterSettingsTypes();
    godotium::RegisterMusicPlayerTypes();
    godotium::RegisterSceneTypes();
  }
}

void UninitializeGodotium(godot::ModuleInitializationLevel) {}
}  // namespace

extern "C" {
GDExtensionBool GDE_EXPORT
godotium_library_init(GDExtensionInterfaceGetProcAddress get_proc_address,
                      GDExtensionClassLibraryPtr library,
                      GDExtensionInitialization* initialization) {
  godot::GDExtensionBinding::InitObject init(get_proc_address, library,
                                             initialization);
  init.register_initializer(InitializeGodotium);
  init.register_terminator(UninitializeGodotium);
  init.set_minimum_library_initialization_level(
      godot::MODULE_INITIALIZATION_LEVEL_SCENE);
  return init.init();
}
}
