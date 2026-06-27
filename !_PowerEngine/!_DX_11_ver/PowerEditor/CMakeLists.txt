cmake_minimum_required(VERSION 3.20)
project(PowerEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt's moc/uic/rcc need these on
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---- Qt 6.10.1, MinGW kit (confirmed from install — no MSVC Qt present) ----
# Adjust this path if your Qt version folder differs.
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1/mingw_64" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

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
)

# nlohmann/json is vendored as a single header (third_party/nlohmann/json.hpp)
# since vcpkg's MSVC-built package can't be linked from this MinGW toolchain.
# It's header-only and template-based, so there's no ABI to worry about.
target_include_directories(PowerEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(PowerEditor PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
)