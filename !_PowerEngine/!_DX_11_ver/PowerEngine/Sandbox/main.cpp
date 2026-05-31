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
    const std::string TEXTURES = ASSETS + "!_test_materials/";
    const std::string MODELS = ASSETS + "!_3D_models/";

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
    font.Load(renderer.GetDevice(), FONTS + "montserrat_bold.ttf", 16.0f);

    // ---- 3D ----
    Engine::Renderer3D renderer3D;
    if (!renderer3D.Init(&renderer, L"Shaders/Mesh.hlsl"))
    {
        LOG_ERROR("Renderer3D init failed.");
        return -1;
    }

    Engine::Grid grid;
    if (!grid.Init(&renderer, L"Shaders/Grid.hlsl", &renderer3D, 100, 1.0f))
    {
        LOG_ERROR("Grid init failed.");
        return -1;
    }

    float fov = 60.0f;

    Engine::Camera3D camera3D;
    camera3D.SetPosition(0.0f, 2.0f, -5.0f);
    camera3D.SetPerspective(fov,
        static_cast<float>(window.GetWidth()) /
        static_cast<float>(window.GetHeight()),
        0.1f, 1000.0f);

    // ---- Models ----
    Engine::Mesh f1;
    bool f1Loaded = f1.Load(renderer.GetDevice(),
        MODELS + "formula_1/f1_mesh.fbx");
    if (!f1Loaded)
        LOG_ERROR("Failed to load the model.");
    f1.SetPosition(0.0f, 0.3f, 0.0f);
    f1.SetScale(0.01f);

    Engine::Mesh bulb;
    bool bulbLoaded = bulb.Load(renderer.GetDevice(),
        MODELS + "bulb/Low_Poly_Light_Bulb.fbx");
    if (!bulbLoaded)
        LOG_ERROR("Failed to load bulb model.");
    bulb.SetScale(1.5f);

    // ---- Timer & Input ----
    Engine::Timer timer;
    timer.Reset();

    Engine::InputManager::Init();
    Engine::GamepadManager::Init();

    bool showInfo = false;

    LOG_INFO("Entering main loop.");

    while (true)
    {
        Engine::InputManager::Update();
        Engine::GamepadManager::Update();

        if (!window.PollEvents())
            break;

        timer.Tick();
        const float dt = timer.DeltaTime();

        // ---- Camera control ----
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

        // FOV with scroll when RMB held
        float scroll = Engine::InputManager::GetMouseScrollDelta();
        if (scroll != 0.0f &&
            Engine::InputManager::IsMouseButtonDown(Engine::MouseButton::Right))
        {
            fov -= scroll * 2.0f;
            if (fov < 10.0f)  fov = 10.0f;
            if (fov > 120.0f) fov = 120.0f;
        }

        if (Engine::InputManager::IsKeyPressed(Engine::Key::F3))
            showInfo = !showInfo;

        if (Engine::InputManager::IsKeyPressed(Engine::Key::R))
        {
            camera3D.SetPosition(0.0f, 2.0f, -5.0f);
            camera3D.SetRotation(0.0f, 0.0f, 0.0f);
            fov = 60.0f;
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
        sun.Direction = { 0.5f, -1.0f, 0.3f };
        sun.Color = { 1.0f, 0.95f, 0.9f };
        sun.Intensity = 3.0f;
        renderer3D.SetDirectionalLight(sun);

        Engine::PointLight redLight;
        redLight.Position = { 3.0f, 2.0f, 0.0f };
        redLight.Color = { 1.0f, 0.2f, 0.1f };
        redLight.Intensity = 30.0f;
        redLight.Radius = 1000.0f;

        Engine::PointLight blueLight;
        blueLight.Position = { -3.0f, 2.0f, 0.0f };
        blueLight.Color = { 0.1f, 0.4f,  1.0f };
        blueLight.Intensity = 30.0f;
        blueLight.Radius = 1000.0f;

        renderer3D.ClearPointLights();
        renderer3D.AddPointLight(redLight);
        renderer3D.AddPointLight(blueLight);

        // ---- Materials ----
        Engine::Material f1Mat;
        f1Mat.Albedo = { 0.8f, 0.1f, 0.1f };
        f1Mat.Metallic = 0.7f;
        f1Mat.Roughness = 0.3f;

        Engine::Material bulbMat;
        bulbMat.Albedo = { 1.0f, 0.9f, 0.6f };
        bulbMat.Metallic = 0.0f;
        bulbMat.Roughness = 0.3f;

        // ---- Render ----
        renderer.BeginFrame(0.13f, 0.13f, 0.13f);

        renderer3D.BeginScene(camera3D);

        grid.Draw(camera3D);

        if (f1Loaded)
            renderer3D.DrawMesh(f1, f1.GetWorldMatrix(), f1Mat);

        if (bulbLoaded)
        {
            bulb.SetPosition(redLight.Position.x,
                redLight.Position.y,
                redLight.Position.z);
            renderer3D.DrawMesh(bulb, bulb.GetWorldMatrix(), bulbMat);

            bulb.SetPosition(blueLight.Position.x,
                blueLight.Position.y,
                blueLight.Position.z);
            renderer3D.DrawMesh(bulb, bulb.GetWorldMatrix(), bulbMat);
        }

        // ---- 2D UI ----
        renderer2D.BeginScene(camera2D);
        renderer2D.BeginScreenSpace();

        if (showInfo)
        {
            auto camPos = camera3D.GetPosition();

            int f1Tris = f1Loaded ? f1.GetIndexCount() / 3 : 0;
            int bulbTris = bulbLoaded ? bulb.GetIndexCount() / 3 : 0;
            int totalTris = f1Tris + bulbTris * 2; // two bulbs drawn
            int totalVerts = totalTris * 3;
            int meshCount = (f1Loaded ? 1 : 0) + (bulbLoaded ? 2 : 0);

            std::string info =
                "FPS:        " + std::to_string((int)timer.FPS()) + "\n" +
                "Frame time: " + std::to_string(
                    timer.DeltaTime() * 1000.0f).substr(0, 5) + " ms\n" +
                "\n" +
                "Camera\n" +
                "  Pos:   (" + std::to_string((int)camPos.x) + ", "
                + std::to_string((int)camPos.y) + ", "
                + std::to_string((int)camPos.z) + ")\n" +
                "  Pitch: " + std::to_string(
                    (int)DirectX::XMConvertToDegrees(camera3D.GetPitch())) + " deg\n" +
                "  Yaw:   " + std::to_string(
                    (int)DirectX::XMConvertToDegrees(camera3D.GetYaw())) + " deg\n" +
                "  FOV:   " + std::to_string((int)fov) + " deg\n" +
                "\n" +
                "Scene\n" +
                "  Meshes:    " + std::to_string(meshCount) + "\n" +
                "  Vertices:  " + std::to_string(totalVerts) + "\n" +
                "  Triangles: " + std::to_string(totalTris) + "\n" +
                "\n" +
                "Controls\n" +
                "  WASD           move\n" +
                "  Space/Ctrl     up/down\n" +
                "  RMB            look\n" +
                "  LShift         sprint\n" +
                "  RMB + Scroll   FOV\n" +
                "  F3             toggle info\n" +
                "  R             reset camera";

            renderer2D.DrawText(font, info, 10.0f, 10.0f, 1.0f, 1.0f, 1.0f);

            renderer2D.DrawText(font, "+", (window.GetWidth() - font.GetFontSize() / 2) / 2, (window.GetHeight() - font.GetFontSize() / 2) / 2);
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