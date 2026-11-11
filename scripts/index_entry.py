#!/usr/bin/env python3
"""Prints the plugin's entry for the oshot plugin index for a release.

    scripts/index_entry.py --dist dist --repo owner/name --tag v1.2.3 --commit <sha>

dist holds the release's libraries, named <anything>-<target>.<so|dll|dylib>
(linux-x86_64, windows-aarch64, ...). The ID, name, ABI and ImGui version come
from the linux-x86_64 library itself, loaded without calling into it, the way
the index checks them. Description and license come from oshot-plugin.toml.

The output is a plugins/<ID>.toml for
https://github.com/Toni500github/oshot-plugin-index. A new plugin needs the
whole file, a new version only its [[versions]] block, added above the older ones.
"""

import argparse
import ctypes
import datetime
import hashlib
import os
import pathlib
import re
import sys
import tomllib

TARGET = re.compile(r"-((?:linux|windows|macos|freebsd)-(?:x86_64|aarch64))\.(?:so|dll|dylib)$")


class Descriptor(ctypes.Structure):
    # The start of oshot_plugin_t, which never changes
    _fields_ = [
        ("struct_size", ctypes.c_uint32),
        ("abi_version", ctypes.c_uint32),
        ("imgui_version", ctypes.c_uint32),
        ("id", ctypes.c_char_p),
        ("name", ctypes.c_char_p),
    ]


def read_descriptor(path):
    # RTLD_LAZY, since a plugin that calls cimgui has symbols only oshot provides
    lib = ctypes.CDLL(str(path.resolve()), mode=os.RTLD_LAZY)
    entry = lib.oshot_plugin_entry
    entry.restype = ctypes.POINTER(Descriptor)
    d = entry().contents
    return d.id.decode(), d.name.decode(), d.abi_version, d.imgui_version


def quote(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--dist", required=True, type=pathlib.Path)
    ap.add_argument("--repo", required=True, help="owner/name on GitHub")
    ap.add_argument("--tag", required=True)
    ap.add_argument("--commit", required=True)
    ap.add_argument("--manifest", default="oshot-plugin.toml", type=pathlib.Path)
    args = ap.parse_args()

    artifacts = {}
    for f in sorted(args.dist.iterdir()):
        m = TARGET.search(f.name)
        if m:
            artifacts[m.group(1)] = f
    if "linux-x86_64" not in artifacts:
        sys.exit("index_entry.py: no linux-x86_64 library in " + str(args.dist))

    plugin_id, name, abi, imgui = read_descriptor(artifacts["linux-x86_64"])
    manifest = tomllib.loads(args.manifest.read_text())
    plugin = next(
        (v for v in manifest.values() if isinstance(v, dict) and v.get("id") == plugin_id),
        None,
    )
    if plugin is None:
        sys.exit(f"index_entry.py: {args.manifest} has no plugin with the library's ID {plugin_id}")

    owner = args.repo.split("/")[0]
    homepage = f"https://github.com/{args.repo}"
    version = args.tag[1:] if args.tag.startswith("v") else args.tag
    licenses = plugin.get("licenses") or ["TODO"]

    out = [
        f"# plugins/{plugin_id}.toml in https://github.com/Toni500github/oshot-plugin-index",
        "# A new plugin needs the whole file, a new version only the [[versions]] block,",
        "# added above the older ones.",
        "schema = 1",
        "",
        f"id          = {quote(plugin_id)}",
        f"name        = {quote(name)}",
        f"description = {quote(plugin.get('description', ''))}",
        f"owners      = [{quote(owner)}]",
        f"homepage    = {quote(homepage)}",
        f"license     = {quote(licenses[0])}",
        "",
        "[[versions]]",
        f"version = {quote(version)}",
        f"date    = {datetime.date.today().isoformat()}",
        f"abi     = {abi}",
        f"imgui   = {imgui}",
        '# changes = "What changed"',
        f"source  = {{ git = {quote(homepage)}, tag = {quote(args.tag)}, commit = {quote(args.commit)} }}",
        "",
        "  [versions.artifacts]",
    ]
    width = max(len(t) for t in artifacts)
    for target, f in artifacts.items():
        sha = hashlib.sha256(f.read_bytes()).hexdigest()
        url = f"{homepage}/releases/download/{args.tag}/{f.name}"
        out.append(f"  {target.ljust(width)} = {{ url = {quote(url)}, sha256 = {quote(sha)} }}")
    print("\n".join(out))


if __name__ == "__main__":
    main()
