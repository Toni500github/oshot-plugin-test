# Builds the plugin against oshot's plugin SDK:
#   cmake -S . -B build -DOSHOT_SDK=<sdk dir> && cmake --build build
# oshotpm passes the SDK's folder in the OSHOT_SDK environment variable. Without
# it, find_package() looks in the usual places, such as the prefix oshot was
# installed to. scripts/fetch_sdk.sh downloads one.
cmake_minimum_required(VERSION 3.16)
project(oshot_plugin C)

set(OSHOT_SDK
    "$ENV{OSHOT_SDK}"
    CACHE PATH
    "oshot plugin SDK, the folder with include/oshot/oshot_plugin.h"
)
find_package(oshot REQUIRED CONFIG HINTS "${OSHOT_SDK}")

# The library goes to build/out, the output-dir in oshot-plugin.toml
oshot_add_plugin(plugin src/plugin.c OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set_target_properties(plugin PROPERTIES C_STANDARD 11)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(plugin PRIVATE -Wall -Wextra)
endif()

# Unit tests on the SDK's mock host, built with -DBUILD_TESTING=ON and run
# with ctest. They're off by default, so installing the plugin skips them
option(BUILD_TESTING "Build the unit tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    find_package(Threads REQUIRED)
    # The plugin's sources are compiled into the test, which calls oshot_plugin_entry() itself
    add_executable(test_plugin tests/test_plugin.c src/plugin.c)
    target_link_libraries(test_plugin PRIVATE oshot::plugin Threads::Threads)
    set_target_properties(test_plugin PROPERTIES C_STANDARD 11)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(test_plugin PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME test_plugin COMMAND test_plugin)
endif()
