add_library(Engine STATIC
    Core/Logger.cpp
    Core/Timer.cpp
    Input/InputManager.cpp
    Input/GamepadManager.cpp
    Platform/Window.cpp
    Renderer/RenderContext.cpp
    Renderer/Shader.cpp
    Renderer/Texture2D.cpp
    Renderer/2D/Renderer2D.cpp
    Renderer/2D/Camera2D.cpp
    Renderer/2D/Font.cpp
    Renderer/3D/Renderer3D.cpp
    Renderer/3D/Camera3D.cpp  
    Renderer/3D/Mesh.cpp
    Renderer/3D/Grid.cpp
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

find_package(Stb REQUIRED)
target_include_directories(Engine PRIVATE ${Stb_INCLUDE_DIR})

find_package(assimp CONFIG REQUIRED)
target_link_libraries(Engine PUBLIC assimp::assimp)

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