#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <cmath>
#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Platform/Window.h"
#include "Renderer/RenderContext.h"
#include "Input/InputManager.h"
#include "Input/GamepadManager.h"

int main()
{
    SDL_SetMainReady();

    LOG_INFO("PowerEngine starting...");

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
        const float dt = timer.DeltaTime();

        Engine::Key k = Engine::InputManager::GetKeyDown();
        if (k != static_cast<Engine::Key>(-1))
            LOG_INFO("Key pressed: {}", Engine::InputManager::KeyToString(k));

        if (Engine::InputManager::IsMouseButtonPressed(Engine::MouseButton::Left))
            LOG_INFO("Left click at ({}, {})",
                Engine::InputManager::GetMouseX(),
                Engine::InputManager::GetMouseY());

        if (Engine::GamepadManager::IsConnected(0))
        {
            if (Engine::GamepadManager::IsButtonPressed(0, Engine::GamepadButton::A))
                LOG_INFO("Gamepad A pressed!");

            if (Engine::GamepadManager::IsButtonPressed(0, Engine::GamepadButton::B))
                Engine::GamepadManager::SetRumble(0, 0.5f, 0.5f);
            if (Engine::GamepadManager::IsButtonReleased(0, Engine::GamepadButton::B))
                Engine::GamepadManager::StopRumble(0);
        }

        renderer.Resize(window.GetWidth(), window.GetHeight());
        renderer.BeginFrame(0.13f, 0.13f, 0.14f);
        renderer.EndFrame();

        static float titleTimer = 0.0f;
        titleTimer += dt;
        if (titleTimer >= 1.0f)
        {
            titleTimer = 0.0f;
            SDL_SetWindowTitle(
                window.GetSDLWindow(),
                ("PowerEngine — " + std::to_string((int)timer.FPS()) + " FPS").c_str()
            );
        }
    }

    Engine::GamepadManager::Shutdown();
    Engine::InputManager::Shutdown();

    LOG_INFO("Shutting down.");
    return 0;
}
