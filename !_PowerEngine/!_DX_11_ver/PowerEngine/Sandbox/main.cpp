#include <iostream>
#include "Core/Logger.h"
#include "Core/Timer.h"

using namespace Engine;

int main()
{
    LOG_INFO("PowerEngine starting...");
    LOG_WARN("This is a warning: {}", 42);
    LOG_ERROR("This is an error: {}", "something failed");

    Engine::Timer timer;
    timer.Reset();


    for (int i = 0; i < 5; i++)
    {
        timer.Tick();
        LOG_INFO("Frame {} | dt: {:.4f}s | FPS: {:.1f}",
            i, timer.DeltaTime(), timer.FPS());
    }

    return 0;
}