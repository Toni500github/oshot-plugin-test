# oshot-plugin-test

A small [oshot](https://github.com/Toni500github/oshot) plugin used to test the plugin system end to end: it uppercases
the extracted text, shows the selection's size, and has a setting to do it automatically, with a prefix.

It needs a `-plugins` build of oshot. The plugin API is still experimental.

## Install

```sh
oshotpm install https://github.com/Toni500github/oshot-plugin-test
```

or from oshot: Tools > Install Plugins..., with the same URL. It's also in the
[plugin index](https://github.com/Toni500github/oshot-plugin-index), so oshot versions that read it can install it
with `oshotpm install io.github.toni500github.plugin-test` or Tools > Browse Plugins. Building it needs git, CMake and a C compiler (on
Windows, MinGW-w64, e.g. MSYS2's UCRT64). Prebuilt libraries for Linux, Windows and macOS are on the
[releases](https://github.com/Toni500github/oshot-plugin-test/releases) page.

## Build by hand

```sh
cmake -S . -B build -DOSHOT_SDK=<oshot's plugin SDK> && cmake --build build
```

`OSHOT_SDK` is the folder with `include/oshot/oshot_plugin.h`: see
[Writing oshot plugins](https://github.com/Toni500github/oshot/blob/main/docs/plugins.md#the-sdk) for where each
oshot package has it, or get it with `scripts/fetch_sdk.sh <oshot commit> sdk`.
