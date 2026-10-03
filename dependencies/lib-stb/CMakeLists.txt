# \file dependencies/lib-stb/CMakeLists.txt
#
# stb_image (https://github.com/nothings/stb) is used to decode PNG files.
# The header is bundled in this directory, see README.md for its version.

option(LOCAL_STB_LIB "Use the bundled stb_image instead of searching the system" ON)

if (NOT LOCAL_STB_LIB)
    message(STATUS "Using system stb_image")

    if (NOT DEFINED STB_INCLUDE_DIR)
        message(FATAL_ERROR "STB_INCLUDE_DIR var not set")
    endif()

    include(CheckIncludeFileCXX)
    set(CMAKE_REQUIRED_INCLUDES "${STB_INCLUDE_DIR}")
    CHECK_INCLUDE_FILE_CXX("stb_image.h" STB_IMAGE_FOUND)
    if (NOT STB_IMAGE_FOUND)
        message(FATAL_ERROR "stb_image.h not found")
    endif()

    set(STB_INCLUDE_DIRS "${STB_INCLUDE_DIR}" PARENT_SCOPE)
    return()
endif()

message(STATUS "Using bundled stb_image")
set(STB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)
