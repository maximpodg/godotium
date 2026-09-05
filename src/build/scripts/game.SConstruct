"""Build gameplay and godot-cpp in an output-local variant directory."""
from pathlib import Path
import json

settings = {key: ARGUMENTS.pop(key) for key in list(ARGUMENTS) if key.startswith("godotium_")}
src = Path(settings["godotium_src"])
objects = Path(settings["godotium_objects"])
SConsignFile(str(objects / ".sconsign.dblite"))
env = SConscript(str(src / "third_party/godot-cpp/SConstruct"),
                 variant_dir=str(objects / "godot-cpp"), duplicate=0)
# Apply cross tools after godot-cpp initializes its platform defaults.
# Its own object targets share this environment and use the same compiler.
for tool in ("CC", "CXX", "AR", "RANLIB", "LINK"):
    if settings.get("godotium_" + tool):
        env[tool] = settings["godotium_" + tool]
env.Append(CPPPATH=[str(src)])
# Pass concrete archive nodes so SCons tracks their contents and links the
# platform-specific GN output, without compiling library sources a second time.
env.Append(LIBS=[env.File(path) for path in
                 json.loads(settings["godotium_static_libraries"])])
sources = [Path(path) for path in json.loads(settings["godotium_sources"])]
objects_list = [env.SharedObject(target=str(objects / source.relative_to(src).with_suffix("")),
                                 source=str(source)) for source in sources]
library = env.SharedLibrary(target=settings["godotium_output"], source=objects_list,
                            SHLIBPREFIX="", SHLIBSUFFIX=Path(settings["godotium_output"]).suffix)
Default(library)
