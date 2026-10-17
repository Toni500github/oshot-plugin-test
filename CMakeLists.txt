# Builds the plugin against oshot's plugin SDK:
#   cmake -S . -B build -DOSHOT_SDK=<sdk dir> && cmake --build build
# oshotpm sets the OSHOT_SDK environment variable when it builds plugins.
cmake_minimum_required(VERSION 3.16)
project(oshot_plugin_test C)

set(OSHOT_SDK "$ENV{OSHOT_SDK}" CACHE PATH "oshot plugin SDK, the folder with include/oshot/oshot_plugin.h")
if(NOT EXISTS "${OSHOT_SDK}/include/oshot/oshot_plugin.h")
    message(FATAL_ERROR "oshot's plugin SDK wasn't found: pass -DOSHOT_SDK=<folder with include/oshot/oshot_plugin.h>")
endif()

add_library(uppercase_plugin MODULE uppercase_plugin.c)
target_include_directories(uppercase_plugin PRIVATE "${OSHOT_SDK}/include")
set_target_properties(uppercase_plugin PROPERTIES C_STANDARD 11 C_VISIBILITY_PRESET hidden)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(uppercase_plugin PRIVATE -Wall -Wextra)
endif()

# The library goes to build/out, for every generator and configuration
set_target_properties(uppercase_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${config}" config)
    set_target_properties(uppercase_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${config} "${CMAKE_BINARY_DIR}/out")
endforeach()

# cimgui comes from oshot: Linux and the BSDs resolve it when oshot loads the
# plugin, macOS needs to be told to, Windows links its import library
if(WIN32)
    find_library(CIMGUI_LIB NAMES cimgui libcimgui PATHS "${OSHOT_SDK}/lib" NO_DEFAULT_PATH REQUIRED)
    target_link_libraries(uppercase_plugin PRIVATE "${CIMGUI_LIB}")
elseif(APPLE)
    target_link_options(uppercase_plugin PRIVATE -undefined dynamic_lookup)
endif()
