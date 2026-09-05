"""Portable implementations of GN's stamp/copy tools."""
from pathlib import Path
import shutil
import sys

operation, *paths = sys.argv[1:]
output = Path(paths[-1])
output.parent.mkdir(parents=True, exist_ok=True)
if operation == "stamp":
    output.touch()
elif operation == "copy":
    shutil.copy2(paths[0], output)
else:
    raise ValueError(operation)
