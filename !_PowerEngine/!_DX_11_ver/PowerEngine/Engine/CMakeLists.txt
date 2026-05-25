add_library(Engine STATIC
    Core/Logger.cpp
    Core/Timer.cpp
    Input/InputManager.cpp
    Input/GamepadManager.cpp
    Platform/Window.cpp
    Renderer/RenderContext.cpp
)

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