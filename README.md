# oshot-plugin-test

A small [oshot](https://github.com/Toni500github/oshot) plugin, and the template to start a plugin from. It uppercases
the extracted text, shows the selection's size, and has a setting to do it automatically, with a prefix. oshot's CI
installs it from the [plugin index](https://github.com/Toni500github/oshot-plugin-index) to test the plugin system
end to end.

It needs a `-plugins` build of oshot. The plugin API is still experimental.

## Install

```sh
oshotpm install io.github.toni500github.plugin-test
```

or from oshot: Tools > Browse Plugins. `oshotpm install https://github.com/Toni500github/oshot-plugin-test` (or Tools >
Install Plugins... with that URL) builds it from source instead, which needs git, CMake and a C compiler. Prebuilt
libraries are on the [releases](https://github.com/Toni500github/oshot-plugin-test/releases) page.

## Start a plugin from this template

Press **Use this template** on GitHub, then in the new repository:

1. Choose an ID, a reverse-domain name like `io.github.<you>.<plugin>`, and set it in `PLUGIN_ID` in
   [`src/plugin.c`](src/plugin.c) and in `id` in [`oshot-plugin.toml`](oshot-plugin.toml). CI fails until the
   template's ID is gone.
2. Set `PLUGIN_NAME`, and in the manifest the `[repository]` name and URL, the plugin's table name, description,
   authors and licenses. If the plugin uses the network, writes files outside its folder or runs programs, declare
   that with a reason in a `[<plugin>.capabilities]` table (see
   [the manifest](https://github.com/Toni500github/oshot/blob/main/docs/plugins.md#the-manifest)). Replace
   [`LICENSE`](LICENSE) if you don't want the Unlicense.
3. Write the plugin in `src/plugin.c` and its tests in [`tests/test_plugin.c`](tests/test_plugin.c).

| File | What it is |
| ---- | ---------- |
| `src/plugin.c` | The plugin: its descriptor, settings and callbacks |
| `tests/test_plugin.c` | Unit tests, run against the SDK's mock host instead of oshot |
| `CMakeLists.txt` | Builds the plugin with `find_package(oshot)`, and the tests with `-DBUILD_TESTING=ON` |
| `oshot-plugin.toml` | The manifest oshotpm installs from |
| `scripts/fetch_sdk.sh` | Downloads oshot's plugin SDK at a given commit |
| `scripts/index_entry.py` | Writes the plugin's entry for the plugin index from a release's libraries |
| `.github/workflows/build.yml` | Builds every target on each push, and releases them for a `v*` tag |

[Writing oshot plugins](https://github.com/Toni500github/oshot/blob/main/docs/plugins.md) documents the API; the
header `oshot/oshot_plugin.h` documents each function.

## Build and test

The build needs oshot's plugin SDK. A `-plugins` oshot comes with it (see
[The SDK](https://github.com/Toni500github/oshot/blob/main/docs/plugins.md#the-sdk) for where), or download the one
CI uses:

```sh
scripts/fetch_sdk.sh <oshot commit> sdk
cmake -S . -B build -DOSHOT_SDK="$PWD/sdk" -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The library ends up in `build/out`. To try it in oshot while working on it, install the folder:

```sh
oshotpm install .
```

and `oshotpm install --force .` after each change.

## Release

Push a tag like `v1.0.0`. The Build workflow then builds the plugin for linux-x86_64, linux-aarch64, windows-x86_64,
windows-aarch64, macos-aarch64 and macos-x86_64, and publishes a release with the libraries, their `SHA256SUMS` and
`index-entry.toml`, which is also in the run's summary. To list the plugin in oshot (Tools > Browse Plugins), open a
pull request on the [plugin index](https://github.com/Toni500github/oshot-plugin-index) that adds that file as
`plugins/<ID>.toml`, or adds its `[[versions]]` block to the file that's there.

`OSHOT_REF` in the workflow is the oshot commit whose SDK the plugin is built with. Move it forward to use API
functions a newer oshot added (and check for them with `OSHOT_HOST_HAS`, so that older oshot versions still load it).
