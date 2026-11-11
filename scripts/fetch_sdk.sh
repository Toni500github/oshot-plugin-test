#!/bin/sh
# Gets oshot's plugin SDK, the folder a plugin build takes as OSHOT_SDK:
#   scripts/fetch_sdk.sh <oshot commit or tag, or an oshot checkout> <sdk dir>
# It holds the headers (include/oshot/) and the CMake package
# (lib/cmake/oshot/oshotConfig.cmake) from oshot's sources at that commit.
#
# With DLLTOOL set (e.g. x86_64-w64-mingw32-dlltool, or "llvm-dlltool -m arm64"),
# it also writes lib/libcimgui.dll.a, the import library Windows plugins that
# call cimgui link. It's made from the functions cimgui.h declares, which are
# the names oshot's libcimgui.dll exports.
set -eu

ref=$1
out=$2

# get <path in oshot's sources> <destination>
get() {
    if [ -d "$ref" ]; then
        cp "$ref/$1" "$2"
    else
        curl -fsSL "https://raw.githubusercontent.com/Toni500github/oshot/$ref/$1" -o "$2"
    fi
}

mkdir -p "$out/include/oshot" "$out/lib/cmake/oshot"
get src/plugins/oshot_plugin.h "$out/include/oshot/oshot_plugin.h"
get src/plugins/oshot_mock_host.h "$out/include/oshot/oshot_mock_host.h"
get include/libs/cimgui.h "$out/include/oshot/cimgui.h"
get src/plugins/sdk/oshotConfig.cmake "$out/lib/cmake/oshot/oshotConfig.cmake"

if [ -n "${DLLTOOL:-}" ]; then
    {
        echo "LIBRARY libcimgui.dll"
        echo "EXPORTS"
        sed -n 's/^CIMGUI_API[^(]*[ *]\([A-Za-z_][A-Za-z0-9_]*\)(.*/\1/p' "$out/include/oshot/cimgui.h" | sort -u
    } > "$out/lib/cimgui.def"
    # DLLTOOL may hold arguments, so it's split on purpose
    # shellcheck disable=SC2086
    $DLLTOOL -d "$out/lib/cimgui.def" -l "$out/lib/libcimgui.dll.a"
fi
