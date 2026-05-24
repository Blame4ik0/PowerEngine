//#include <filesystem>
//#define SDL_MAIN_HANDLED
//#include <SDL2/SDL.h>
//#include <cmath>
//#include "Core/Logger.h"
//#include "Core/Timer.h"
//#include "Platform/Window.h"
//#include "Renderer/RenderContext.h"
//#include "Renderer/Renderer2D.h"
//#include "Input/InputManager.h"
//#include "Input/GamepadManager.h"
//
//int main()
//{
//    LOG_INFO("Working directory: {}", std::filesystem::current_path().string());
//
//    SDL_SetMainReady();
//
//    LOG_INFO("PowerEngine starting...");
//
//    Engine::WindowProps props;
//    props.Title = "PowerEngine v0.1.1";
//    props.Width = 1280;
//    props.Height = 720;
//    props.VSync = true;
//    props.RefreshRate = 170;
//
//    Engine::Window window(props);
//
//    Engine::RenderContext renderer
//    (
//        window.GetHWND(),
//        window.GetWidth(),
//        window.GetHeight(),
//        window.GetVSync(),
//        window.GetRefreshRate()
//    );
//
//    Engine::Renderer2D renderer2D;
//    if (!renderer2D.Init(&renderer, L"Shaders/Quad.hlsl"))
//        return -1;
//
//    Engine::Timer timer;
//    timer.Reset();
//
//    Engine::InputManager::Init();
//    Engine::GamepadManager::Init();
//
//    LOG_INFO("Entering main loop.");
//
//    while (true)
//    {
//        Engine::InputManager::Update();   // snapshot previous, read keyboard
//        Engine::GamepadManager::Update(); // snapshot previous, read gamepads
//
//        if (!window.PollEvents())         // pump events — fills mouse button state
//            break;
//
//        timer.Tick();
//        const float dt = timer.DeltaTime();
//
//		Engine::MouseButton mb = Engine::InputManager::GetMouseButtonDown();
//        if (mb != static_cast<Engine::MouseButton>(-1))
//        {
//			if (mb == Engine::MouseButton::Left)
//				LOG_INFO("Mouse button pressed: Left at ({}, {})", Engine::InputManager::GetMouseX(), Engine::InputManager::GetMouseY());
//			else if (mb == Engine::MouseButton::Right)
//				LOG_INFO("Mouse button pressed: Right at ({}, {})", Engine::InputManager::GetMouseX(), Engine::InputManager::GetMouseY());
//			else if (mb == Engine::MouseButton::Middle)
//				LOG_INFO("Mouse button pressed: Middle at ({}, {})", Engine::InputManager::GetMouseX(), Engine::InputManager::GetMouseY());
//            else if (mb == Engine::MouseButton::X1)
//                LOG_INFO("Mouse button pressed: X1 at ({}, {})", Engine::InputManager::GetMouseX(), Engine::InputManager::GetMouseY());
//            else if (mb == Engine::MouseButton::X2)
//                LOG_INFO("Mouse button pressed: X2 at ({}, {})", Engine::InputManager::GetMouseX(), Engine::InputManager::GetMouseY());
//        }
//
//        Engine::Key k = Engine::InputManager::GetKeyDown();
//        if (k != static_cast<Engine::Key>(-1))
//            LOG_INFO("Key pressed: {}", Engine::InputManager::KeyToString(k));
//
//        if (Engine::GamepadManager::IsConnected(0))
//        {
//            if (Engine::GamepadManager::IsButtonPressed(0, Engine::GamepadButton::A))
//                LOG_INFO("Gamepad A pressed!");
//
//            float lx = Engine::GamepadManager::GetLeftStickX(0);
//            float ly = Engine::GamepadManager::GetLeftStickY(0);
//            if (std::abs(lx) > 0.0f || std::abs(ly) > 0.0f)
//                LOG_INFO("Left stick: ({:.2f}, {:.2f})", lx, ly);
//
//            if (Engine::GamepadManager::IsButtonPressed(0, Engine::GamepadButton::B))
//                Engine::GamepadManager::SetRumble(0, 0.5f, 0.5f);
//            if (Engine::GamepadManager::IsButtonReleased(0, Engine::GamepadButton::B))
//                Engine::GamepadManager::StopRumble(0);
//        }
//
//        renderer.Resize(window.GetWidth(), window.GetHeight());
//        renderer.BeginFrame(0.1f, 0.1f, 0.1f);
//
//        renderer2D.DrawQuad(100.0f, 100.0f, 200.0f, 150.0f, 1.0f, 0.4f, 0.1f);
//        renderer2D.DrawQuad(400.0f, 200.0f, 100.0f, 100.0f, 0.2f, 0.6f, 1.0f);
//        renderer2D.Flush();
//
//        renderer.EndFrame();
//
//        static float titleTimer = 0.0f;
//        titleTimer += dt;
//        if (titleTimer >= 1.0f)
//        {
//            titleTimer = 0.0f;
//            SDL_SetWindowTitle(
//                window.GetSDLWindow(),
//                ("PowerEngine — " + std::to_string((int)timer.FPS()) + " FPS").c_str()
//            );
//        }
//    }
//
//    renderer2D.Shutdown();
//    Engine::InputManager::Shutdown();
//    Engine::GamepadManager::Shutdown();
//
//    LOG_INFO("Shutting down.");
//    return 0;
//}

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <filesystem>
#include <cmath>
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
        const float dt = timer.DeltaTime();

        // Input tests
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

        renderer2D.DrawQuad(0.0f, 0.0f, 640.0f, 360.0f, 1.0f, 0.0f, 0.0f, 1.0f);
        renderer2D.DrawQuad(400.0f, 200.0f, 100.0f, 100.0f, 0.2f, 0.6f, 1.0f);
        renderer2D.Flush();

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

    renderer2D.Shutdown();
    Engine::GamepadManager::Shutdown();
    Engine::InputManager::Shutdown();

    LOG_INFO("Shutting down.");
    return 0;
}