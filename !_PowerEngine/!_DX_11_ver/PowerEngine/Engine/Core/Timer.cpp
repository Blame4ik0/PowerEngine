#include "Timer.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Engine
{
    Timer::Timer()
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        m_frequency = freq.QuadPart;
        Reset();
    }

    void Timer::Reset()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        m_startTime = now.QuadPart;
        m_prevTime = now.QuadPart;
        m_currentTime = now.QuadPart;
        m_deltaTime = 0.0f;
        m_fpsSmoothed = 0.0f;
    }

    void Timer::Tick()
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        m_currentTime = now.QuadPart;

        m_deltaTime = static_cast<float>(m_currentTime - m_prevTime)
            / static_cast<float>(m_frequency);

        if (m_deltaTime > 0.25f) m_deltaTime = 0.25f;

        constexpr float alpha = 0.05f;
        if (m_deltaTime > 0.0f)
        {
            float rawFPS = 1.0f / m_deltaTime;
            m_fpsSmoothed = (m_fpsSmoothed == 0.0f)
                ? rawFPS
                : m_fpsSmoothed * (1.0f - alpha) + rawFPS * alpha;
        }

        m_prevTime = m_currentTime;
    }

    float Timer::DeltaTime() const { return m_deltaTime; }

    float Timer::TotalTime()  const
    {
        return static_cast<float>(m_currentTime - m_startTime)
            / static_cast<float>(m_frequency);
    }

    float Timer::FPS() const { return m_fpsSmoothed; }
}