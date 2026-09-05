"""Compare embedded Windows icon resources with the source ICO, using stdlib."""
import struct
from pathlib import Path


def check_windows_icon(executable, icon):
    data = Path(executable).read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    optional = pe + 24
    if data[pe:pe + 4] != b"PE\0\0" or struct.unpack_from("<H", data, optional)[0] != 0x20B:
        raise RuntimeError("Expected a PE32+ image")
    sections = optional + optional_size

    def offset(rva):
        for index in range(count):
            start = sections + index * 40
            size, address, raw_size, raw = struct.unpack_from("<IIII", data, start + 8)
            if address <= rva < address + max(size, raw_size):
                return raw + rva - address
        raise RuntimeError("Resource RVA is outside PE sections")

    resource_rva = struct.unpack_from("<I", data, optional + 112 + 2 * 8)[0]
    if not resource_rva:
        raise RuntimeError("Windows executable has no resources")
    base = offset(resource_rva)

    def children(relative):
        table = base + relative
        named, numbered = struct.unpack_from("<HH", data, table + 12)
        return [struct.unpack_from("<II", data, table + 16 + index * 8)
                for index in range(named + numbered)]

    def leaves(relative, depth=0):
        if depth > 3:
            raise RuntimeError("Unexpected resource tree depth")
        for _, child in children(relative):
            if child & 0x80000000:
                yield from leaves(child & 0x7FFFFFFF, depth + 1)
            else:
                rva, size = struct.unpack_from("<II", data, base + child)
                start = offset(rva)
                yield data[start:start + size]

    types = dict(children(0))
    if 3 not in types or 14 not in types:
        raise RuntimeError("Missing Windows ICON or GROUP_ICON resource")
    embedded = set(leaves(types[3] & 0x7FFFFFFF))
    ico = Path(icon).read_bytes()
    reserved, kind, images = struct.unpack_from("<HHH", ico)
    if (reserved, kind) != (0, 1) or images < 1:
        raise RuntimeError("Invalid source ICO")
    for index in range(images):
        size, start = struct.unpack_from("<II", ico, 6 + index * 16 + 8)
        if ico[start:start + size] not in embedded:
            raise RuntimeError(f"Windows EXE is missing Godotium icon size #{index}")
