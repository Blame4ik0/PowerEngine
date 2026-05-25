#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <filesystem>
#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Platform/Window.h"
#include "Renderer/RenderContext.h"
#include "Renderer/Renderer2D.h"
#include "Input/InputManager.h"
#include "Input/GamepadManager.h"

int main()
{
    SDL_SetMainReady();

    LOG_INFO("PowerEngine starting...");
    LOG_INFO("Working directory: {}", std::filesystem::current_path().string());

    Engine::WindowProps props;
    props.Title = "PowerEngine";
    props.Width = 1280;
    props.Height = 720;
    props.VSync = true;
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

    Engine::InputManager::Init();
    Engine::GamepadManager::Init();

    LOG_INFO("Entering main loop.");

    while (true)
    {
        Engine::InputManager::Update();
        Engine::GamepadManager::Update();

        if (!window.PollEvents())
            break;

        timer.Tick();

        renderer.Resize(window.GetWidth(), window.GetHeight());
        renderer2D.OnResize(window.GetWidth(), window.GetHeight());
        renderer.BeginFrame(0.1f, 0.1f, 0.1f, 1.0f);

        // === DRAW QUADS (now with better visibility) ===
        renderer2D.DrawQuad(400.0f, 200.0f, 480.0f, 320.0f, 1.0f, 0.2f, 0.2f, 1.0f);  // Big red
        renderer2D.DrawQuad(100.0f, 100.0f, 200.0f, 150.0f, 0.2f, 1.0f, 0.2f, 1.0f);  // Green
        renderer2D.DrawQuad(800.0f, 400.0f, 250.0f, 180.0f, 0.2f, 0.2f, 1.0f, 1.0f);  // Blue

        renderer.EndFrame();
    }

    renderer2D.Shutdown();
    Engine::InputManager::Shutdown();
    Engine::GamepadManager::Shutdown();

    LOG_INFO("Shutting down.");
    return 0;
}