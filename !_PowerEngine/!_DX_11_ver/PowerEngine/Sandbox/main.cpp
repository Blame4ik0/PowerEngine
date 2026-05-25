#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
<<<<<<< HEAD
#include <filesystem>
=======
>>>>>>> parent of dfbd2bf3 (Inputs system)
#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Platform/Window.h"
#include "Renderer/RenderContext.h"
<<<<<<< HEAD
#include "Renderer/Renderer2D.h"
#include "Input/InputManager.h"
#include "Input/GamepadManager.h"
=======
>>>>>>> parent of dfbd2bf3 (Inputs system)

int main()
{
    SDL_SetMainReady();

    LOG_INFO("PowerEngine starting...");
    LOG_INFO("Working directory: {}", std::filesystem::current_path().string());

    Engine::WindowProps props;
    props.Title = "PowerEngine";
    props.Width = 1280;
    props.Height = 720;
<<<<<<< HEAD
    props.VSync = true;
=======
    props.VSync = false;
>>>>>>> parent of dfbd2bf3 (Inputs system)
    props.RefreshRate = 144;

    Engine::Window window(props);

    Engine::RenderContext renderer(
        window.GetHWND(),
        window.GetWidth(),
        window.GetHeight(),
        window.GetVSync(),
        window.GetRefreshRate()
    );

    Engine::Renderer2D renderer2D;
    if (!renderer2D.Init(&renderer, L"Shaders/Quad.hlsl"))
    {
        LOG_ERROR("Failed to initialize Renderer2D.");
        return -1;
    }

    Engine::Timer timer;
    timer.Reset();

    LOG_INFO("Entering main loop.");

    while (window.PollEvents())
    {
<<<<<<< HEAD
        Engine::InputManager::Update();
        Engine::GamepadManager::Update();

        if (!window.PollEvents())
            break;

        timer.Tick();

        renderer.Resize(window.GetWidth(), window.GetHeight());
        renderer2D.OnResize(window.GetWidth(), window.GetHeight());
        renderer.BeginFrame(0.1f, 0.1f, 0.1f, 1.0f);
=======
        timer.Tick();
        const float dt = timer.DeltaTime();

        renderer.Resize(window.GetWidth(), window.GetHeight());
        renderer.BeginFrame(0.13f, 0.13f, 0.14f);

        // draw calls go here later

        renderer.EndFrame();
>>>>>>> parent of dfbd2bf3 (Inputs system)

        // === DRAW QUADS (now with better visibility) ===
        renderer2D.DrawQuad(400.0f, 200.0f, 480.0f, 320.0f, 1.0f, 0.2f, 0.2f, 1.0f);  // Big red
        renderer2D.DrawQuad(100.0f, 100.0f, 200.0f, 150.0f, 0.2f, 1.0f, 0.2f, 1.0f);  // Green
        renderer2D.DrawQuad(800.0f, 400.0f, 250.0f, 180.0f, 0.2f, 0.2f, 1.0f, 1.0f);  // Blue

        renderer.EndFrame();
    }

<<<<<<< HEAD
    renderer2D.Shutdown();
    Engine::InputManager::Shutdown();
    Engine::GamepadManager::Shutdown();

=======
>>>>>>> parent of dfbd2bf3 (Inputs system)
    LOG_INFO("Shutting down.");
    return 0;
}