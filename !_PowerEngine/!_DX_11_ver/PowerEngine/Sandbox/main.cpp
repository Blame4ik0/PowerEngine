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
#include "Renderer/3D/Light.h"
#include "Renderer/3D/Grid.h"

#include "Input/InputManager.h"
#include "Input/GamepadManager.h"

int main()
{
    const std::string ASSETS = "../../../../../../!_ASSETS/";
    const std::string FONTS = ASSETS + "!_fonts/";

    SDL_SetMainReady();

    LOG_INFO("PowerEngine starting...");

    Engine::WindowProps props;
    props.Title = "PowerEngine - Shadow Test";
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
    font.Load(renderer.GetDevice(), FONTS + "montserrat_bold.ttf", 16.0f);

    // ---- 3D ----
    Engine::Renderer3D renderer3D;
    if (!renderer3D.Init(&renderer, L"Shaders/Mesh.hlsl"))
    {
        LOG_ERROR("Renderer3D init failed.");
        return -1;
    }

    Engine::Grid grid;
    if (!grid.Init(&renderer, L"Shaders/Grid.hlsl", &renderer3D, 20, 1.0f))
    {
        LOG_ERROR("Grid init failed.");
        return -1;
    }

    float fov = 60.0f;
    float lightIntensity = 3.0f;

    Engine::Camera3D camera3D;
    camera3D.SetPosition(0.0f, 8.0f, -12.0f);
    camera3D.SetRotation(0.4f, 0.0f, 0.0f);
    camera3D.SetPerspective(fov,
        static_cast<float>(window.GetWidth()) /
        static_cast<float>(window.GetHeight()),
        0.1f, 1000.0f);

    // ---- Primitives ----
    Engine::Mesh ground;
    ground.CreatePlane(renderer.GetDevice(), 30.0f, 30.0f);
    ground.SetPosition(0.0f, 0.0f, 0.0f);

    Engine::Mesh cube1;
    cube1.CreateCube(renderer.GetDevice(), 2.0f);
    cube1.SetPosition(0.0f, 1.0f, 0.0f);

    Engine::Mesh cube2;
    cube2.CreateCube(renderer.GetDevice(), 1.5f);
    cube2.SetPosition(4.0f, 0.75f, 2.0f);

    Engine::Mesh sphere1;
    sphere1.CreateSphere(renderer.GetDevice(), 1.0f, 24, 24);
    sphere1.SetPosition(-4.0f, 1.0f, 0.0f);

    Engine::Mesh sphere2;
    sphere2.CreateSphere(renderer.GetDevice(), 1.5f, 24, 24);
    sphere2.SetPosition(2.0f, 1.5f, 4.0f);

    // ---- Timer & Input ----
    Engine::Timer timer;
    timer.Reset();

    Engine::InputManager::Init();
    Engine::GamepadManager::Init();

    bool showInfo = true;
    bool showCrosshair = false;
    bool showGrid = true;
    bool showShadows = true;

    LOG_INFO("Entering main loop.");

    while (true)
    {
        Engine::InputManager::Update();
        Engine::GamepadManager::Update();

        if (!window.PollEvents())
            break;

        timer.Tick();
        const float dt = timer.DeltaTime();

        // ---- Camera Control ----
        if (Engine::InputManager::IsMouseButtonPressed(Engine::MouseButton::Right))
            SDL_SetRelativeMouseMode(SDL_TRUE);
        if (Engine::InputManager::IsMouseButtonReleased(Engine::MouseButton::Right))
            SDL_SetRelativeMouseMode(SDL_FALSE);

        if (Engine::InputManager::IsMouseButtonDown(Engine::MouseButton::Right))
        {
            float dx = Engine::InputManager::GetMouseDeltaX() * 0.003f;
            float dy = Engine::InputManager::GetMouseDeltaY() * 0.003f;
            camera3D.Rotate(dy, dx, 0.0f);
        }

        float multiplier = Engine::InputManager::IsKeyDown(Engine::Key::LShift)
            ? 15.0f : 5.0f;
        float speed = multiplier * dt;

        if (Engine::InputManager::IsKeyDown(Engine::Key::W))     camera3D.Move(0, 0, speed);
        if (Engine::InputManager::IsKeyDown(Engine::Key::S))     camera3D.Move(0, 0, -speed);
        if (Engine::InputManager::IsKeyDown(Engine::Key::A))     camera3D.Move(-speed, 0, 0);
        if (Engine::InputManager::IsKeyDown(Engine::Key::D))     camera3D.Move(speed, 0, 0);
        if (Engine::InputManager::IsKeyDown(Engine::Key::Space)) camera3D.Move(0, speed, 0);
        if (Engine::InputManager::IsKeyDown(Engine::Key::LCtrl)) camera3D.Move(0, -speed, 0);

        float scroll = Engine::InputManager::GetMouseScrollDelta();
        if (scroll != 0.0f && Engine::InputManager::IsMouseButtonDown(Engine::MouseButton::Right))
        {
            fov -= scroll * 2.0f;
            fov = std::max(10.0f, std::min(120.0f, fov));
        }
        else if (scroll != 0.0f && Engine::InputManager::IsKeyDown(Engine::Key::L))
        {
            lightIntensity += scroll * 0.5f;
            lightIntensity = std::max(0.0f, lightIntensity);
        }

        // ---- Key Toggles ----
        if (Engine::InputManager::IsKeyPressed(Engine::Key::F3))
            showInfo = !showInfo;
        if (Engine::InputManager::IsKeyPressed(Engine::Key::C))
            showCrosshair = !showCrosshair;
        if (Engine::InputManager::IsKeyPressed(Engine::Key::G))
            showGrid = !showGrid;
        if (Engine::InputManager::IsKeyPressed(Engine::Key::F4))
        {
            showShadows = !showShadows;
            renderer3D.EnableShadows(showShadows);
        }
        if (Engine::InputManager::IsKeyPressed(Engine::Key::R))
        {
            camera3D.SetPosition(0.0f, 8.0f, -12.0f);
            camera3D.SetRotation(0.4f, 0.0f, 0.0f);
            fov = 60.0f;
        }
        if (Engine::InputManager::IsKeyDown(Engine::Key::R) &&
            Engine::InputManager::IsKeyPressed(Engine::Key::L))
        {
            lightIntensity = 3.0f;
        }

        // ---- Resize ----
        renderer.Resize(window.GetWidth(), window.GetHeight());
        camera2D.SetViewSize(
            static_cast<float>(window.GetWidth()),
            static_cast<float>(window.GetHeight()));
        camera3D.SetPerspective(fov,
            static_cast<float>(window.GetWidth()) /
            static_cast<float>(window.GetHeight()),
            0.1f, 1000.0f);

        // ---- Lighting ----
        Engine::DirectionalLight sun;
        sun.Direction = { -0.5f, -1.0f, 0.5f };
        sun.Color = { 1.0f,  0.95f, 0.9f };
        sun.Intensity = lightIntensity;
        renderer3D.SetDirectionalLight(sun);

        Engine::PointLight whiteLight;
        whiteLight.Position = { 0.0f, 5.0f, -3.0f };
        whiteLight.Color = { 1.0f, 1.0f,  1.0f };
        whiteLight.Intensity = lightIntensity * 10.0f;
        whiteLight.Radius = 15.0f;

        renderer3D.ClearPointLights();
        renderer3D.AddPointLight(whiteLight);

        // ---- Materials ----
        Engine::Material groundMat;
        groundMat.Albedo = { 0.4f, 0.4f, 0.4f };
        groundMat.Metallic = 0.0f;
        groundMat.Roughness = 0.9f;

        Engine::Material redMat;
        redMat.Albedo = { 0.8f, 0.1f, 0.1f };
        redMat.Metallic = 0.0f;
        redMat.Roughness = 0.6f;

        Engine::Material goldMat;
        goldMat.Albedo = { 1.0f, 0.76f, 0.33f };
        goldMat.Metallic = 1.0f;
        goldMat.Roughness = 0.2f;

        Engine::Material blueMat;
        blueMat.Albedo = { 0.1f, 0.3f, 0.9f };
        blueMat.Metallic = 0.0f;
        blueMat.Roughness = 0.4f;

        Engine::Material ironMat;
        ironMat.Albedo = { 0.56f, 0.57f, 0.58f };
        ironMat.Metallic = 1.0f;
        ironMat.Roughness = 0.7f;

        // ---- Render ----
        renderer.BeginFrame(0.1f, 0.1f, 0.12f);

        renderer3D.BeginScene(camera3D);

        // ======================
        // SHADOW PASS
        // ======================
        renderer3D.BeginShadowPass();

        // Draw ONLY geometry - no need for full materials here
        renderer3D.DrawMesh(ground, ground.GetWorldMatrix());
        renderer3D.DrawMesh(cube1, cube1.GetWorldMatrix());
        renderer3D.DrawMesh(cube2, cube2.GetWorldMatrix());
        renderer3D.DrawMesh(sphere1, sphere1.GetWorldMatrix());
        renderer3D.DrawMesh(sphere2, sphere2.GetWorldMatrix());

        renderer3D.EndShadowPass();

        // ======================
        // MAIN PASS
        // ======================
        if (showGrid)
            grid.Draw(camera3D);

        // Now draw with materials
        renderer3D.DrawMesh(ground, ground.GetWorldMatrix(), groundMat);
        renderer3D.DrawMesh(cube1, cube1.GetWorldMatrix(), redMat);
        renderer3D.DrawMesh(cube2, cube2.GetWorldMatrix(), goldMat);
        renderer3D.DrawMesh(sphere1, sphere1.GetWorldMatrix(), blueMat);
        renderer3D.DrawMesh(sphere2, sphere2.GetWorldMatrix(), ironMat);

        // ---- 2D UI ----
        renderer2D.BeginScene(camera2D);
        renderer2D.BeginScreenSpace();

        if (showInfo)
        {
            auto camPos = camera3D.GetPosition();
            std::string info =
                "FPS:        " + std::to_string((int)timer.FPS()) + "\n"
                "Frame time: " + std::to_string(timer.DeltaTime() * 1000.0f).substr(0, 5) + " ms\n"
                "\n"
                "Camera\n"
                "  Pos:   (" + std::to_string((int)camPos.x) + ", "
                + std::to_string((int)camPos.y) + ", "
                + std::to_string((int)camPos.z) + ")\n"
                "  Pitch: " + std::to_string((int)DirectX::XMConvertToDegrees(camera3D.GetPitch())) + " deg\n"
                "  Yaw:   " + std::to_string((int)DirectX::XMConvertToDegrees(camera3D.GetYaw())) + " deg\n"
                "  FOV:   " + std::to_string((int)fov) + " deg\n"
                "\n"
                "Shadows:   " + std::string(showShadows ? "ON" : "OFF") + " (F4)\n"
                "Sun Int:   " + std::to_string(lightIntensity).substr(0, 4) + "\n"
                "\n"
                "Controls\n"
                "  WASD + Space/Ctrl  Move\n"
                "  RMB                Look\n"
                "  RMB + Scroll       FOV\n"
                "  L + Scroll         Light Intensity\n"
                "  R                  Reset Camera\n"
                "  R + L              Reset Light\n"
                "  F3                 Toggle Info\n"
                "  C                  Crosshair\n"
                "  G                  Grid\n"
                "  F4                 Shadows";

            renderer2D.DrawText(font, info, 10.0f, 10.0f, 1.0f, 1.0f, 1.0f);
        }
        else
        {
            renderer2D.DrawText(font, "F3 - Toggle Info", 10.0f, 10.0f, 0.7f, 0.7f, 0.7f);
        }

        if (showCrosshair)
        {
            renderer2D.DrawText(font, "+",
                static_cast<float>(window.GetWidth()) / 2.0f - 4.0f,
                static_cast<float>(window.GetHeight()) / 2.0f - 8.0f,
                1.0f, 1.0f, 1.0f);
        }

        renderer2D.Flush();

        renderer.EndFrame();
    }

    grid.Shutdown();
    renderer3D.Shutdown();
    renderer2D.Shutdown();
    Engine::GamepadManager::Shutdown();
    Engine::InputManager::Shutdown();

    LOG_INFO("Shutting down.");
    return 0;
}
