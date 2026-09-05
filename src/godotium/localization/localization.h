#ifndef GODOTIUM_LOCALIZATION_LOCALIZATION_H_
#define GODOTIUM_LOCALIZATION_LOCALIZATION_H_

#include "godot_cpp/variant/packed_string_array.hpp"
#include "godot_cpp/variant/string.hpp"

namespace godotium {
godot::PackedStringArray GetLocales();
godot::String FindSupportedLocale(const godot::String& requested);
}  // namespace godotium
#endif  // GODOTIUM_LOCALIZATION_LOCALIZATION_H_
