#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <cmath>

#include "Core/Logger.h"
#include "Core/Timer.h"

#include "Platform/Window.h"

#include "Renderer/RenderContext.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Camera2D.h"
#include "Renderer/Texture2D.h"

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

    Engine::RenderContext renderer
    (
        window.GetHWND(),
        window.GetWidth(),
        window.GetHeight(),
        window.GetVSync(),
        window.GetRefreshRate()
    );

    Engine::Renderer2D renderer2D;
    if (!renderer2D.Init(&renderer, L"Shaders/Polygon.hlsl"))
    {
        LOG_ERROR("Renderer2D init failed.");
        return -1;
    }

    Engine::Camera2D camera;
    camera.SetViewSize(
        static_cast<float>(window.GetWidth()),
        static_cast<float>(window.GetHeight()));

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

        float camSpeed = 200.0f * dt;
        float camX = camera.GetX();
        float camY = camera.GetY();

        if (Engine::InputManager::IsKeyDown(Engine::Key::W) || Engine::InputManager::IsKeyDown(Engine::Key::Up)) camY -= camSpeed;
        if (Engine::InputManager::IsKeyDown(Engine::Key::S) || Engine::InputManager::IsKeyDown(Engine::Key::Down)) camY += camSpeed;
        if (Engine::InputManager::IsKeyDown(Engine::Key::A) || Engine::InputManager::IsKeyDown(Engine::Key::Left)) camX -= camSpeed;
        if (Engine::InputManager::IsKeyDown(Engine::Key::D) || Engine::InputManager::IsKeyDown(Engine::Key::Right)) camX += camSpeed;

        float scroll = Engine::InputManager::GetMouseScrollDelta();
        if (scroll != 0.0f)
            camera.SetZoom(camera.GetZoom() + scroll * 0.1f);

        camera.SetPosition(camX, camY);

        renderer.Resize(window.GetWidth(), window.GetHeight());
        renderer2D.OnResize(window.GetWidth(), window.GetHeight());

        // Update camera view size on resize
        camera.SetViewSize
        (
            static_cast<float>(window.GetWidth()),
            static_cast<float>(window.GetHeight())
        );

        renderer.BeginFrame(0.13f, 0.13f, 0.13f);

        renderer2D.BeginScene(camera);

        // Solid color quads
        renderer2D.DrawQuad(100.0f, 100.0f, 300.0f, 200.0f, 1.0f, 0.3f, 0.0f);
        renderer2D.DrawQuad(500.0f, 300.0f, 150.0f, 150.0f, 0.0f, 0.5f, 1.0f);

        // Per-vertex color quad - gradient
        renderer2D.DrawQuad(
            Engine::Vertex2D{ 800.0f, 100.0f, 1.0f, 0.0f, 0.0f, 1.0f },  // TL red
            Engine::Vertex2D{ 1000.0f, 100.0f, 0.0f, 1.0f, 0.0f, 1.0f }, // TR green
            Engine::Vertex2D{ 1000.0f, 300.0f, 0.0f, 0.0f, 1.0f, 1.0f }, // BR blue
            Engine::Vertex2D{ 800.0f, 300.0f, 1.0f, 1.0f, 0.0f, 1.0f }   // BL yellow
        );

        // Polygon
        renderer2D.DrawPolygon(
            Engine::Vertex2D{ 400.0f, 500.0f, 1.0f, 0.0f, 0.0f, 1.0f },
            Engine::Vertex2D{ 600.0f, 500.0f, 0.0f, 1.0f, 0.0f, 1.0f },
            Engine::Vertex2D{ 500.0f, 650.0f, 0.0f, 0.0f, 1.0f, 1.0f }
        );

        renderer2D.Flush();

        renderer.EndFrame();
    }

    renderer2D.Shutdown();
    Engine::GamepadManager::Shutdown();
    Engine::InputManager::Shutdown();

    LOG_INFO("Shutting down.");
    return 0;
}
