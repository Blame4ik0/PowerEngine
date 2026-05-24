add_library(Engine STATIC
    Core/Logger.cpp
    Core/Timer.cpp
    Platform/Window.cpp
    Renderer/RenderContext.cpp
    Input/InputManager.cpp
    Input/GamepadManager.cpp
    Renderer/Renderer2D.cpp
    Renderer/Shader.cpp
 "Core/Logger.h" "Core/Timer.h" "Platform/Window.h" "Renderer/RenderContext.h" "Input/InputManager.h" "Input/GamepadManager.h" "Renderer/Renderer2D.h"  "Renderer/Shader.h")

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

add_custom_command(TARGET Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Renderer/Shaders
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Shaders
)