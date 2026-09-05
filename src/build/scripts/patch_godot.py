"""Apply the small, pinned engine fixes required by Godotium."""
from pathlib import Path
from build_utils import file_lock


def replace_once(path, original, patched):
    content = path.read_text()
    if patched in content:
        return
    if content.count(original) != 1:
        raise RuntimeError(f"Pinned Godot patch no longer applies: {path}")
    path.write_text(content.replace(original, patched))


def apply_patches(source):
    source = Path(source)
    with file_lock(source / ".godotium-build.lock"):
        original = "void EditorHelp::_gen_extensions_docs() {\n"
        replace_once(source / "editor/doc/editor_help.cpp", original, original + (
            "\t// Import/export can finish before this deferred callback runs.\n"
            "\t// Main::cleanup may already have released the documentation.\n"
            "\tif (!doc) {\n\t\treturn;\n\t}\n"
        ))
        original = (
            "void AudioServer::finish() {\n"
            "\tfor (int i = 0; i < AudioDriverManager::get_driver_count(); i++) {\n"
            "\t\tAudioDriverManager::get_driver(i)->finish();\n\t}\n"
        )
        replace_once(source / "servers/audio/audio_server.cpp", original, original + (
            "\n\t// Drivers have stopped: pending fades can no longer retire playbacks.\n"
            "\t// Release them before ObjectDB and resource caches are torn down.\n"
            "\tfor (AudioStreamPlaybackListNode *playback : playback_list) {\n"
            "\t\t_delete_stream_playback_list_node(playback);\n\t}\n"
            "\t_cleanup_lists();\n"
        ))
