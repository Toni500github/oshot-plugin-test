#!/bin/sh
# Downloads oshot's plugin SDK headers at a given oshot commit or tag:
#   scripts/fetch_sdk.sh <oshot ref> <sdk dir>
# With DLLTOOL set (e.g. x86_64-w64-mingw32-dlltool), also writes the cimgui
# import library Windows plugins link, from the functions cimgui.h declares:
# the same names oshot's libcimgui.dll exports.
set -eu

ref=$1
out=$2
base="https://raw.githubusercontent.com/Toni500github/oshot/$ref"

mkdir -p "$out/include/oshot"
curl -fsSL "$base/src/plugins/oshot_plugin.h" -o "$out/include/oshot/oshot_plugin.h"
curl -fsSL "$base/include/libs/cimgui/cimgui.h" -o "$out/include/oshot/cimgui.h"

if [ -n "${DLLTOOL:-}" ]; then
    mkdir -p "$out/lib"
    {
        echo "LIBRARY libcimgui.dll"
        echo "EXPORTS"
        sed -n 's/^CIMGUI_API[^(]*[ *]\([A-Za-z_][A-Za-z0-9_]*\)(.*/\1/p' "$out/include/oshot/cimgui.h" | sort -u
    } > "$out/lib/cimgui.def"
    "$DLLTOOL" -d "$out/lib/cimgui.def" -l "$out/lib/libcimgui.dll.a"
fi
