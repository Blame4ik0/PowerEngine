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
    props.Title = "PowerEngine v0.1.1";
    props.Width = 1280;
    props.Height = 720;
    props.VSync = true;
    props.RefreshRate = 170;

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
        Engine::InputManager::Update();   // snapshot previous, read keyboard
        Engine::GamepadManager::Update(); // snapshot previous, read gamepads

        if (!window.PollEvents())         // pump events — fills mouse button state
            break;

        timer.Tick();
        const float dt = timer.DeltaTime();

		Engine::MouseButton mb = Engine::InputManager::GetMouseButtonDown();
        if (mb != static_cast<Engine::MouseButton>(-1))
        {
			if (mb == Engine::MouseButton::Left)
				LOG_INFO("Mouse button pressed: Left at ({}, {})", Engine::InputManager::GetMouseX(), Engine::InputManager::GetMouseY());
			else if (mb == Engine::MouseButton::Right)
				LOG_INFO("Mouse button pressed: Right at ({}, {})", Engine::InputManager::GetMouseX(), Engine::InputManager::GetMouseY());
			else if (mb == Engine::MouseButton::Middle)
				LOG_INFO("Mouse button pressed: Middle at ({}, {})", Engine::InputManager::GetMouseX(), Engine::InputManager::GetMouseY());
        }

        Engine::Key k = Engine::InputManager::GetKeyDown();
        if (k != static_cast<Engine::Key>(-1))
            LOG_INFO("Key pressed: {}", Engine::InputManager::KeyToString(k));

        if (Engine::GamepadManager::IsConnected(0))
        {
            if (Engine::GamepadManager::IsButtonPressed(0, Engine::GamepadButton::A))
                LOG_INFO("Gamepad A pressed!");

            float lx = Engine::GamepadManager::GetLeftStickX(0);
            float ly = Engine::GamepadManager::GetLeftStickY(0);
            if (std::abs(lx) > 0.0f || std::abs(ly) > 0.0f)
                LOG_INFO("Left stick: ({:.2f}, {:.2f})", lx, ly);

            if (Engine::GamepadManager::IsButtonPressed(0, Engine::GamepadButton::B))
                Engine::GamepadManager::SetRumble(0, 0.5f, 0.5f);
            if (Engine::GamepadManager::IsButtonReleased(0, Engine::GamepadButton::B))
                Engine::GamepadManager::StopRumble(0);
        }

        renderer.Resize(window.GetWidth(), window.GetHeight());
        renderer.BeginFrame(0.85f, 0.41f, 0.27f);
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

    Engine::InputManager::Shutdown();
    Engine::GamepadManager::Shutdown();

    LOG_INFO("Shutting down.");
    return 0;
}