cmake_minimum_required(VERSION 3.20)
project(zadanieNatalia LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(zadanieNatalia
        src/main.cpp
        src/ObjLoader.cpp
        external/glad/src/glad.c
)

target_include_directories(zadanieNatalia PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/glfw-3.4.bin.WIN64/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glm/glm
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)

# GLFW dla MinGW
target_link_directories(zadanieNatalia PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/glfw-3.4.bin.WIN64/lib-mingw-w64
)

# Dla MinGW linkowanie zwykle wygląda tak:
target_link_libraries(zadanieNatalia PRIVATE
        glfw3
        opengl32
        gdi32
        user32
        shell32
)

# Jeśli build krzyczy, że nie znajduje glfw3, to zmienisz na:
# target_link_libraries(zadanieNatalia PRIVATE glfw3dll opengl32 gdi32 user32 shell32)

