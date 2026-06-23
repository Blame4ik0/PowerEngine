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
#include "Renderer/3D/Scene.h"
#include "Renderer/3D/Components.h"

#include "Input/InputManager.h"
#include "Input/GamepadManager.h"

#include <entt/entt.hpp>

// Force discrete GPU on laptops with both Intel and NVIDIA/AMD
extern "C" { __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }
extern "C" { __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1; }

int main()
{
    const std::string ASSETS = "../../../../../../!_ASSETS/";
    const std::string FONTS = ASSETS + "!_fonts/";
    const std::string TEXTURES = ASSETS + "!_test_materials/";
    const std::string MODELS = ASSETS + "!_3D_models/";
    const std::string F1_TEX = MODELS + "formula_1/Substance_SpecGloss/Right_ones/";
    const std::string CONTAINER = MODELS + "container/";

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
    font.Load(renderer.GetDevice(), renderer.GetDeviceContext(), FONTS + "montserrat_bold.ttf", 16.0f);

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
    float lightIntensity = 3.0f;
    bool showInfo = false;
    bool showCrosshair = false;
    bool showGrid = true;
    bool showShadows = true;
    int shadowQuality = 3;

    Engine::Camera3D camera3D;
    camera3D.SetPosition(0.0f, 4.0f, -10.0f);
    camera3D.SetPerspective(fov,
        static_cast<float>(window.GetWidth()) /
        static_cast<float>(window.GetHeight()),
        0.1f, 1000.0f);

    // ================================================================
    //  ECS Scene — everything lives here now
    // ================================================================
    Engine::Scene scene;

    // ---- Floor ----
    auto floorMesh = std::make_shared<Engine::Mesh>();
    floorMesh->CreatePlane(renderer.GetDevice(), 30.0f, 30.0f);

    auto floorEntity = scene.CreateEntity("Floor");
    scene.Registry().emplace<Engine::MeshComponent>(floorEntity,
        Engine::MeshComponent{ floorMesh, false }); // false = use MaterialComponent

    Engine::Material floorMat;
    floorMat.Albedo = { 0.15f, 0.15f, 0.15f };
    floorMat.Metallic = 0.0f;
    floorMat.Roughness = 0.0f;
    scene.Registry().emplace<Engine::MaterialComponent>(floorEntity,
        Engine::MaterialComponent{ floorMat });

    // ---- Sword (center) ----
    auto swordMesh = std::make_shared<Engine::Mesh>();
    bool swordLoaded = swordMesh->Load(renderer.GetDevice(), renderer.GetDeviceContext(),
        MODELS + "sword/sword.glb");
    if (!swordLoaded) LOG_ERROR("Failed to load Sword model.");

    auto swordEntity = scene.CreateEntity("Sword");
    {
        auto& t = scene.Registry().get<Engine::TransformComponent>(swordEntity);
        t.Position = { 0.0f, 1.8f, 0.0f };
        t.Scale = { 0.03f, 0.03f, 0.03f };

        scene.Registry().emplace<Engine::MeshComponent>(swordEntity,
            Engine::MeshComponent{ swordMesh, true }); // true = DrawMeshAuto (embedded materials)

        Engine::MaterialOverride swordOv;
        swordOv.overrideMetallic = true;  swordOv.metallic = 1.0f;
        swordOv.overrideRoughness = true;  swordOv.roughness = 0.2f;
        scene.Registry().emplace<Engine::MaterialOverrideComponent>(swordEntity,
            Engine::MaterialOverrideComponent{ swordOv, 1 });
    }

    // ---- F1 car (back-left) ----
    auto f1Mesh = std::make_shared<Engine::Mesh>();
    bool f1Loaded = f1Mesh->Load(renderer.GetDevice(), renderer.GetDeviceContext(),
        MODELS + "formula_1/f1_mesh.obj");
    if (!f1Loaded) LOG_ERROR("Failed to load F1 model.");

    auto f1Entity = scene.CreateEntity("F1Car");
    {
        auto& t = scene.Registry().get<Engine::TransformComponent>(f1Entity);
        t.Position = { -6.0f, 0.3f, 4.0f };
        t.Scale = { 0.01f, 0.01f, 0.01f };

        scene.Registry().emplace<Engine::MeshComponent>(f1Entity,
            Engine::MeshComponent{ f1Mesh, false }); // uses MaterialComponent below

        Engine::Material f1Mat;
        f1Mat.Albedo = { 1.0f, 1.0f, 1.0f };
        f1Mat.Metallic = 1.0f;
        f1Mat.Roughness = 0.5f;
        f1Mat.AlbedoMap = F1_TEX + "formula1_DefaultMaterial_Diffuse.png";
        f1Mat.SpecularMap = F1_TEX + "formula1_DefaultMaterial_Specular.png";
        f1Mat.GlossinessMap = F1_TEX + "formula1_DefaultMaterial_Glossiness.png";
        scene.Registry().emplace<Engine::MaterialComponent>(f1Entity,
            Engine::MaterialComponent{ f1Mat });
    }

    // ---- Angel (front-left) ----
    auto angelMesh = std::make_shared<Engine::Mesh>();
    bool angelLoaded = angelMesh->Load(renderer.GetDevice(), renderer.GetDeviceContext(),
        MODELS + "angel/scene.gltf");
    if (!angelLoaded) LOG_ERROR("Failed to load angel model.");

    auto angelEntity = scene.CreateEntity("Angel");
    {
        auto& t = scene.Registry().get<Engine::TransformComponent>(angelEntity);
        t.Position = { -6.0f, 0.1f, -4.0f };
        t.Rotation = { -90.0f, 180.0f, 0.0f };
        t.Scale = { 0.4f, 0.4f, 0.4f };

        scene.Registry().emplace<Engine::MeshComponent>(angelEntity,
            Engine::MeshComponent{ angelMesh, true }); // DrawMeshAuto, embedded materials
    }

    // ---- Container (back-right) ----
    auto containerMesh = std::make_shared<Engine::Mesh>();
    bool containerLoaded = containerMesh->Load(renderer.GetDevice(), renderer.GetDeviceContext(),
        CONTAINER + "Container.fbx");
    if (!containerLoaded) LOG_ERROR("Failed to load container model.");

    auto containerEntity = scene.CreateEntity("Container");
    {
        auto& t = scene.Registry().get<Engine::TransformComponent>(containerEntity);
        t.Position = { 6.0f, 2.0f, 4.0f };
        t.Rotation = { 0.0f, 0.0f, 90.0f };
        t.Scale = { 0.01f, 0.01f, 0.01f };

        scene.Registry().emplace<Engine::MeshComponent>(containerEntity,
            Engine::MeshComponent{ containerMesh, false }); // uses MaterialComponent below

        Engine::Material containerMat;
        containerMat.Albedo = { 1.0f, 1.0f, 1.0f };
        containerMat.Metallic = 0.0f;
        containerMat.Roughness = 0.5f;
        containerMat.AlbedoMap = CONTAINER + "Container_DiffuseMap.jpg";
        containerMat.SpecularMap = CONTAINER + "Container_SpecularMap.jpg";
        containerMat.NormalMap = CONTAINER + "Container_NormalsMap.png";
        scene.Registry().emplace<Engine::MaterialComponent>(containerEntity,
            Engine::MaterialComponent{ containerMat });
    }

    // ---- Knight (front-right) ----
    auto knightMesh = std::make_shared<Engine::Mesh>();
    bool knightLoaded = knightMesh->Load(renderer.GetDevice(), renderer.GetDeviceContext(),
        MODELS + "knight/scene.gltf");
    if (!knightLoaded) LOG_ERROR("Failed to load knight model.");

    auto knightEntity = scene.CreateEntity("Knight");
    {
        auto& t = scene.Registry().get<Engine::TransformComponent>(knightEntity);
        t.Position = { 6.0f, 0.1f, -4.0f };
        t.Rotation = { 180.0f, 0.0f, 0.0f };
        t.Scale = { 0.08f, 0.08f, 0.08f };

        scene.Registry().emplace<Engine::MeshComponent>(knightEntity,
            Engine::MeshComponent{ knightMesh, true }); // DrawMeshAuto, embedded materials
    }

    // ---- Bulbs (follow point lights) ----
    // Kept as direct Mesh/draw calls since their position is dynamic,
    // tied every frame to the point light positions below.
    Engine::Mesh bulb;
    bool bulbLoaded = bulb.Load(renderer.GetDevice(), renderer.GetDeviceContext(),
        MODELS + "bulb/Low_Poly_Light_Bulb.fbx");
    if (!bulbLoaded) LOG_ERROR("Failed to load bulb model.");
    bulb.SetScale(1.5f);

    // ---- Timer & Input ----

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

        // ---- Camera control ----
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
        if (scroll != 0.0f &&
            Engine::InputManager::IsMouseButtonDown(Engine::MouseButton::Right))
        {
            fov -= scroll * 2.0f;
            fov = std::max(10.0f, std::min(120.0f, fov));
        }
        else if (scroll != 0.0f &&
            Engine::InputManager::IsKeyDown(Engine::Key::L))
        {
            lightIntensity += scroll * 0.5f;
            lightIntensity = std::max(0.0f, lightIntensity);
        }

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
            camera3D.SetPosition(0.0f, 4.0f, -10.0f);
            camera3D.SetRotation(0.0f, 0.0f, 0.0f);
            fov = 60.0f;
        }
        if (Engine::InputManager::IsKeyDown(Engine::Key::LAlt) &&
            Engine::InputManager::IsKeyPressed(Engine::Key::L))
            lightIntensity = 3.0f;
        if (Engine::InputManager::IsKeyPressed(Engine::Key::Up)) shadowQuality = std::min(shadowQuality + 1, 4);
        if (Engine::InputManager::IsKeyPressed(Engine::Key::Down)) shadowQuality = std::max(shadowQuality - 1, 0);

        if (Engine::InputManager::IsKeyDown(Engine::Key::LAlt) && Engine::InputManager::IsKeyDown(Engine::Key::U))
        {
            renderer3D.SetShadowQuality(static_cast<Engine::ShadowQuality>(shadowQuality));
            renderer3D.SetPointShadowQuality(static_cast<Engine::PointShadowQuality>(shadowQuality));
            LOG_INFO("Shadow quality set to {}.", shadowQuality);
            LOG_WARN("!! Current changes only work with shadows. TBE later !!");
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
        sun.Intensity = lightIntensity;
        renderer3D.SetDirectionalLight(sun);

        Engine::PointLight redLight;
        redLight.Position = { 3.0f, 2.0f, 4.0f };
        redLight.Color = { 1.0f, 0.2f, 0.1f };
        redLight.Intensity = 400.0f * lightIntensity;
        redLight.Radius = 15.0f;

        Engine::PointLight blueLight;
        blueLight.Position = { -3.0f, 2.0f, -4.0f };
        blueLight.Color = { 0.1f, 0.4f,  1.0f };
        blueLight.Intensity = 400.0f * lightIntensity;
        blueLight.Radius = 15.0f;

        renderer3D.ClearPointLights();
        renderer3D.AddPointLight(redLight);
        renderer3D.AddPointLight(blueLight);

        Engine::Material redBulbMat;
        redBulbMat.Albedo = { 1.0f, 0.2f, 0.1f };
        redBulbMat.Metallic = 0.0f;
        redBulbMat.Roughness = 0.3f;
        redBulbMat.NormalMap = MODELS + "bulb/Textures/#LMP0003_Textures_2k/#LMP0003_Textures_NRML_2k.png";

        Engine::Material blueBulbMat;
        blueBulbMat.Albedo = { 0.1f, 0.4f, 1.0f };
        blueBulbMat.Metallic = 0.0f;
        blueBulbMat.Roughness = 0.3f;
        blueBulbMat.NormalMap = MODELS + "bulb/Textures/#LMP0003_Textures_2k/#LMP0003_Textures_NRML_2k.png";

        // ---- Render ----
        renderer.BeginFrame(0.13f, 0.13f, 0.13f);

        renderer3D.BeginScene(camera3D);

        // Shadow pass
        renderer3D.BeginShadowPass();
        scene.Draw(renderer3D);
        renderer3D.EndShadowPass();

        // Point shadow pass
        renderer3D.BeginPointShadowPass();
        for (int light = 0; light < 2; light++)
        {
            for (int face = 0; face < 6; face++)
            {
                renderer3D.RenderPointShadowFace(light, face);
                scene.Draw(renderer3D);
            }
        }
        renderer3D.EndPointShadowPass();

        // Main pass
        if (showGrid)
            grid.Draw(camera3D);
        scene.Draw(renderer3D);
        if (bulbLoaded)
        {
            bulb.SetPosition(redLight.Position.x,
                redLight.Position.y,
                redLight.Position.z);
            renderer3D.DrawMesh(bulb, bulb.GetWorldMatrix(), redBulbMat);

            bulb.SetPosition(blueLight.Position.x,
                blueLight.Position.y,
                blueLight.Position.z);
            renderer3D.DrawMesh(bulb, bulb.GetWorldMatrix(), blueBulbMat);
        }

        // ---- 2D UI ----
        renderer2D.BeginScene(camera2D);
        renderer2D.BeginScreenSpace();

        if (showInfo)
        {
            auto camPos = camera3D.GetPosition();

            int f1Tris = f1Loaded ? f1Mesh->GetIndexCount() / 3 : 0;
            int containerTris = containerLoaded ? containerMesh->GetIndexCount() / 3 : 0;
            int swordTris = swordLoaded ? swordMesh->GetIndexCount() / 3 : 0;
            int angelTris = angelLoaded ? angelMesh->GetIndexCount() / 3 : 0;
            int knightTris = knightLoaded ? knightMesh->GetIndexCount() / 3 : 0;
            int bulbTris = bulbLoaded ? bulb.GetIndexCount() / 3 : 0;
            int floorTris = floorMesh->GetIndexCount() / 3;
            int totalTris = f1Tris + containerTris + swordTris + angelTris +
                knightTris + bulbTris * 2 + floorTris;
            int totalVerts = totalTris * 3;
            int meshCount = (f1Loaded ? 1 : 0) + (containerLoaded ? 1 : 0) +
                (swordLoaded ? 1 : 0) + (angelLoaded ? 1 : 0) +
                (knightLoaded ? 1 : 0) + (bulbLoaded ? 2 : 0) + 1;

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
                "  Culled:    " + std::to_string(renderer3D.GetCulledCount()) + "\n" +
                "  Shadows:   " + std::string(showShadows ? "on" : "off") + "\n" +
                "  Sun intensity: " + std::to_string(lightIntensity).substr(0, 4) + "\n" +
                "  Shadow quality: " + std::to_string(shadowQuality) + "\n" +
                "\n" +
                "Controls\n" +
                "  WASD           move\n" +
                "  Space/Ctrl     up/down\n" +
                "  RMB            look\n" +
                "  LShift         sprint\n" +
                "  RMB+Scroll     FOV\n" +
                "  L+Scroll       light intensity\n" +
                "  LAlt+L         reset light\n" +
                "  R              reset camera\n" +
                "  C              crosshair\n" +
                "  G              grid\n" +
                "  F4             shadows\n" +
                "  Up/Down        shadow quality\n" +
                "  LAlt+U         update and reinit (to apply changes)";

            renderer2D.DrawText(font, info, 10.0f, 10.0f, 1.0f, 1.0f, 1.0f);
        }
        else
        {
            renderer2D.DrawText(font, "F3  info", 10.0f, 10.0f,
                0.6f, 0.6f, 0.6f);
        }

        if (showCrosshair)
            renderer2D.DrawText(font, "+",
                static_cast<float>(window.GetWidth()) / 2.0f - 4.0f,
                static_cast<float>(window.GetHeight()) / 2.0f - 8.0f,
                1.0f, 1.0f, 1.0f);

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