cmake_minimum_required(VERSION 3.12)
project(snowshooter)

set(CMAKE_CXX_STANDARD 14)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# Bump up warning levels appropriately for clang, gcc & msvc and build in debug mode
message("CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wconversion -pedantic -std=c++11")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_WARNING_LEVEL 4)

    # Replace the default /W flag to /W4 for higher verbosity.
    if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif ()

    # Workaround for MSVC development libraries in the University's laboratories, where access to
    # C:\ or C:\MinGW are forbidden. This serves to let anybody compile under restricted access.
    if (NOT BUILD_LOCAL)
        if (EXISTS "${snowshooter_SOURCE_DIR}/deps" AND IS_DIRECTORY "${snowshooter_SOURCE_DIR}/deps")
            set(__DEPS_FOLDER 1)
        else ()
            set(__DEPS_FOLDER 0)
        endif ()

        set(BUILD_LOCAL "${__DEPS_FOLDER}" CACHE PATH "Assign this to 1 to use a local /deps/ folder.")
        unset(__DEPS_FOLDER)
    endif (NOT BUILD_LOCAL)
endif ()

# Print the compiler flags for debugging purposes.
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")

# Set the cmake module path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${snowshooter_SOURCE_DIR}/cmake/")
set(BIN_DIR ${snowshooter_SOURCE_DIR}/bin)

# If the generator is MSVC and a deps folder exists, try to link the libraries.
if (BUILD_LOCAL EQUAL 1)
    file(GLOB SDL2_DIR "${snowshooter_SOURCE_DIR}/deps/SDL2-*")
    file(GLOB SDL2_IMAGE_DIR "${snowshooter_SOURCE_DIR}/deps/SDL2_image-*")
    file(GLOB SDL2_TTF_DIR "${snowshooter_SOURCE_DIR}/deps/SDL2_ttf-*")
    file(GLOB SDL2_MIXER_DIR "${snowshooter_SOURCE_DIR}/deps/SDL2_mixer-*")

    message("SDL2_DIR = ${SDL2_DIR}")
    message("SDL2_IMAGE_DIR = ${SDL2_IMAGE_DIR}")
    message("SDL2_TTF_DIR = ${SDL2_TTF_DIR}")
    message("SDL2_MIXER_DIR = ${SDL2_MIXER_DIR}")
endif ()

# Find all SDL2 libraries.
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR})

add_executable(snowshooter main.cpp)

# If a deps folder was used, it is most likely that OS' scoped libraries will be tried to be used even if they are not
# available, so we copy the .dll files.
if (BUILD_LOCAL EQUAL 1)
    file(GLOB_RECURSE DEPENDENCY_DLLS
            "${SDL2_TTF_DIR}/*.dll"
            "${SDL2_MIXER_DIR}/*.dll"
            "${SDL2_IMAGE_DIR}/*.dll"
            "${SDL2_DIR}/*.dll")
    file(COPY ${DEPENDENCY_DLLS} DESTINATION "${BIN_DIR}")
endif ()

# Link the libraries and install them.
target_link_libraries(snowshooter ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_MIXER_LIBRARIES})
install(TARGETS snowshooter RUNTIME DESTINATION ${BIN_DIR})
