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
endif ()

# Print the compiler flags for debugging purposes.
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
message("CMAKE_CXX_FLAGS_DEBUG = ${CMAKE_CXX_FLAGS_DEBUG}")
message("CMAKE_CXX_FLAGS_RELEASE = ${CMAKE_CXX_FLAGS_RELEASE}")

# Set the cmake module path.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${snowshooter_SOURCE_DIR}/cmake/")
set(BIN_DIR ${snowshooter_SOURCE_DIR}/bin)

# Find all SDL2 libraries.
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_net REQUIRED)
include_directories(${SDL2_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR} ${SDL2_NET_INCLUDE_DIR})

file(COPY assets DESTINATION .)
add_executable(snowshooter src/main.cpp src/Game.cpp src/Game.h src/Texture.cpp src/Texture.h src/Vector2D.h src/Input.cpp src/Input.h src/GameObject.cpp src/GameObject.h src/Scene.cpp src/Scene.h src/TimePool.cpp src/TimePool.h src/Constants.h src/SnowShooterError.h src/SDLError.h src/Font.cpp src/Font.h src/FontManager.cpp src/FontManager.h src/AudioManager.cpp src/AudioManager.h src/SDLAudioManager.cpp src/SDLAudioManager.h src/TextureManager.cpp src/TextureManager.h src/ResourceManager.h src/SceneMachine.cpp src/SceneMachine.h src/MenuScene.cpp src/MenuScene.h src/EventListener.cpp src/EventListener.h src/GameManager.cpp src/GameManager.h src/Tileset.cpp src/Tileset.h src/Server.cpp src/Server.h src/Client.cpp src/Client.h)

# Link the libraries and install them.
target_link_libraries(snowshooter ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${SDL2_TTF_LIBRARIES} ${SDL2_MIXER_LIBRARY} ${SDL2_NET_LIBRARY})
install(TARGETS snowshooter RUNTIME DESTINATION ${BIN_DIR})
