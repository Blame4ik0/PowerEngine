add_library(Engine STATIC
    Core/Logger.cpp
    Core/Timer.cpp
    Platform/Window.cpp
    Renderer/RenderContext.cpp
    Input/InputManager.cpp
    Input/GamepadManager.cpp
 "Core/Logger.h" "Core/Timer.h" "Platform/Window.h" "Renderer/RenderContext.h" "Input/InputManager.h" "Input/GamepadManager.h")

target_include_directories(Engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Engine
    PUBLIC
        SDL2::SDL2
        SDL2::SDL2main
        d3d11.lib
        dxgi.lib
        d3dcompiler.lib
        xinput.lib
)

target_compile_definitions(Engine PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
)