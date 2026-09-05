#include "godotium/localization/localization.h"

#include "godot_cpp/classes/translation_server.hpp"

namespace godotium {
using godot::PackedStringArray;
using godot::String;
using godot::TranslationServer;

PackedStringArray GetLocales() {
  PackedStringArray locales =
      TranslationServer::get_singleton()->get_loaded_locales();
  locales.sort();
  return locales;
}

String FindSupportedLocale(const String& requested) {
  auto* translations = TranslationServer::get_singleton();
  String best = "en";
  int best_score = 0;
  for (const String& locale : GetLocales()) {
    const int score = translations->compare_locales(requested, locale);
    if (score > best_score) {
      best = locale;
      best_score = score;
    }
  }
  return best;
}

}  // namespace godotium
