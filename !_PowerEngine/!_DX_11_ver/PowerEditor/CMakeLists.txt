cmake_minimum_required(VERSION 3.20)

# vcpkg toolchain must be set BEFORE project() — this is the only reliable
# way to guarantee it loads regardless of Qt Creator's CMake argument handling.
# Hardcoded here because Qt Creator consistently mangles the path when set
# via the kit UI (strips the drive letter colon, making the path invalid).
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/!_DEV/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE FILEPATH "")
endif()
set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "" FORCE)

project(PowerEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(assimp CONFIG REQUIRED)

add_executable(PowerEditor
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/HierarchyPanel.cpp
    src/HierarchyPanel.h
    src/InspectorPanel.cpp
    src/InspectorPanel.h
    src/SceneDocument.cpp
    src/SceneDocument.h
    src/RunSettingsDialog.cpp
    src/RunSettingsDialog.h
    src/ViewportWidget.cpp
    src/ViewportWidget.h
    src/MeshBoundsCache.cpp
    src/MeshBoundsCache.h
    src/ModelPreviewDialog.cpp
    src/ModelPreviewDialog.h
    src/AssetPathResolver.cpp
    src/AssetPathResolver.h
)

target_include_directories(PowerEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(PowerEditor PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    assimp::assimp
)
