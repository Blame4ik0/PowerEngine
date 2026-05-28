#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <cmath>

#include "Core/Logger.h"
#include "Core/Timer.h"

#include "Platform/Window.h"

#include "Renderer/RenderContext.h"
#include "Renderer/Texture2D.h"

#include "Renderer/2D/Renderer2D.h"
#include "Renderer/2D/Camera2D.h"
#include "Renderer/2D/Font.h"

#include "Renderer/3D/Renderer3D.h"
#include "Renderer/3D/Camera3D.h"
#include "Renderer/3D/Mesh.h"

#include "Input/InputManager.h"
#include "Input/GamepadManager.h"

int main()
{
    const std::string ASSETS = "../../../../../../!_ASSETS/";
    const std::string FONTS = ASSETS + "!_fonts/";
    const std::string TEXTURES = ASSETS + "!_test_materials/";

    SDL_SetMainReady();

    LOG_INFO("PowerEngine starting...");

    Engine::WindowProps props;
    props.Title = "PowerEngine";
    props.Width = 1280;
    props.Height = 720;
    props.VSync = false;
    props.RefreshRate = 500;

    Engine::Window window(props);

    Engine::RenderContext renderer(
        window.GetHWND(),
        window.GetWidth(),
        window.GetHeight(),
        window.GetVSync(),
        window.GetRefreshRate()
    );

    // ---- 2D ----
    Engine::Renderer2D renderer2D;
    if (!renderer2D.Init(&renderer, L"Shaders/Polygon.hlsl"))
    {
        LOG_ERROR("Renderer2D init failed.");
        return -1;
    }

    Engine::Camera2D camera2D;
    camera2D.SetViewSize(
        static_cast<float>(window.GetWidth()),
        static_cast<float>(window.GetHeight()));

    Engine::Font font;
    font.Load(renderer.GetDevice(), FONTS + "montserrat_bold.ttf", 24.0f);

    // ---- 3D ----
    Engine::Renderer3D renderer3D;
    if (!renderer3D.Init(&renderer, L"Shaders/Mesh.hlsl"))
    {
        LOG_ERROR("Renderer3D init failed.");
        return -1;
    }

    Engine::Camera3D camera3D;
    camera3D.SetPosition(0.0f, 0.0f, -5.0f);
    camera3D.SetPerspective(
        60.0f,
        static_cast<float>(window.GetWidth()) /
        static_cast<float>(window.GetHeight()),
        0.1f, 1000.0f);

    Engine::Mesh cube;
    cube.CreateCube(renderer.GetDevice(), 1.0f);

    // ---- Timer & Input ----
    Engine::Timer timer;
    timer.Reset();

    Engine::InputManager::Init();
    Engine::GamepadManager::Init();

    static float rotation = 0.0f;

    LOG_INFO("Entering main loop.");

    while (true)
    {
        Engine::InputManager::Update();
        Engine::GamepadManager::Update();

        if (!window.PollEvents())
            break;

        timer.Tick();
        const float dt = timer.DeltaTime();

        // ---- 3D camera control (hold right mouse) ----
        if (Engine::InputManager::IsMouseButtonDown(Engine::MouseButton::Right))
        {
            float dx = Engine::InputManager::GetMouseDeltaX() * 0.003f;
            float dy = Engine::InputManager::GetMouseDeltaY() * 0.003f;
            camera3D.Rotate(dy, dx, 0.0f);

            float speed = 5.0f * dt;
            if (Engine::InputManager::IsKeyDown(Engine::Key::W)) camera3D.Move(0, 0, speed);
            if (Engine::InputManager::IsKeyDown(Engine::Key::S)) camera3D.Move(0, 0, -speed);
            if (Engine::InputManager::IsKeyDown(Engine::Key::A)) camera3D.Move(-speed, 0, 0);
            if (Engine::InputManager::IsKeyDown(Engine::Key::D)) camera3D.Move(speed, 0, 0);
            if (Engine::InputManager::IsKeyDown(Engine::Key::E)) camera3D.Move(0, speed, 0);
            if (Engine::InputManager::IsKeyDown(Engine::Key::Q)) camera3D.Move(0, -speed, 0);
        }

        // ---- Resize ----
        renderer.Resize(window.GetWidth(), window.GetHeight());

        camera2D.SetViewSize(
            static_cast<float>(window.GetWidth()),
            static_cast<float>(window.GetHeight()));

        camera3D.SetPerspective(
            60.0f,
            static_cast<float>(window.GetWidth()) /
            static_cast<float>(window.GetHeight()),
            0.1f, 1000.0f);

        // ---- Update ----
        rotation += 1.0f * dt;

        // ---- Render ----
        renderer.BeginFrame(0.13f, 0.13f, 0.13f);

        // 3D pass
        renderer3D.BeginScene(camera3D);
        renderer3D.DrawMesh(cube, 0.0f, 0.0f, 0.0f, rotation, rotation * 0.7f, 0.0f);
        renderer3D.DrawMesh(cube, 3.0f, 0.0f, 0.0f, 0.0f, rotation, 0.0f);
        renderer3D.DrawMesh(cube, -3.0f, 0.0f, 0.0f, rotation * 0.5f, 0.0f, rotation);

        // 2D screen space pass
        renderer2D.BeginScene(camera2D);
        renderer2D.BeginScreenSpace();

        renderer2D.DrawText(font,
            "FPS: " + std::to_string((int)timer.FPS()),
            10.0f, 10.0f,
            1.0f, 1.0f, 1.0f);

        renderer2D.DrawText(font,
            "RMB + WASD to move | QE up/down",
            10.0f, 40.0f,
            0.7f, 0.7f, 0.7f);

        renderer2D.Flush();

        renderer.EndFrame();
    }

    renderer3D.Shutdown();
    renderer2D.Shutdown();
    Engine::GamepadManager::Shutdown();
    Engine::InputManager::Shutdown();

    LOG_INFO("Shutting down.");
    return 0;
}