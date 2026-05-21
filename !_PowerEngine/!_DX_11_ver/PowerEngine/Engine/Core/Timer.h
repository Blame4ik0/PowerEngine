#pragma once

namespace Engine
{
    class Timer
    {
    public:
        Timer();

        void  Reset();
        void  Tick();

        float DeltaTime() const;
        float TotalTime() const;
        float FPS()       const;

    private:
        long long m_frequency = 0;
        long long m_prevTime = 0;
        long long m_startTime = 0;
        long long m_currentTime = 0;

        float m_deltaTime = 0.0f;
        float m_fpsSmoothed = 0.0f;
    };
}