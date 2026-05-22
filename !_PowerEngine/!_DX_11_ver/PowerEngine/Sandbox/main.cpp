#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Platform/Window.h"
#include "Renderer/RenderContext.h"

int main()
{
    SDL_SetMainReady();

    LOG_INFO("PowerEngine starting...");

    Engine::WindowProps props;
    props.Title = "PowerEngine";
    props.Width = 1280;
    props.Height = 720;
    props.VSync = false;
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

    LOG_INFO("Entering main loop.");

    while (window.PollEvents())
    {
        timer.Tick();
        const float dt = timer.DeltaTime();

        renderer.Resize(window.GetWidth(), window.GetHeight());
        renderer.BeginFrame(0.13f, 0.13f, 0.14f);

        // draw calls go here later

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

    LOG_INFO("Shutting down.");
    return 0;
}