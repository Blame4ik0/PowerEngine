#pragma once
#include <array>
#include <string>

namespace Engine
{
    enum class GamepadButton
    {
        DPadUp = 0x0001,
        DPadDown = 0x0002,
        DPadLeft = 0x0004,
        DPadRight = 0x0008,
        Start = 0x0010,
        Back = 0x0020,
        LeftThumb = 0x0040,
        RightThumb = 0x0080,
        LeftBumper = 0x0100,
        RightBumper = 0x0200,
        A = 0x1000,
        B = 0x2000,
        X = 0x4000,
        Y = 0x8000
    };

    struct GamepadState
    {
        float leftStickX = 0.0f;
        float leftStickY = 0.0f;
        float rightStickX = 0.0f;
        float rightStickY = 0.0f;
        float leftTrigger = 0.0f;
        float rightTrigger = 0.0f;

        unsigned short buttons = 0;
        unsigned short previousButtons = 0;

        bool connected = false;
    };

    class GamepadManager
    {
    public:
        static constexpr int MaxGamepads = 4;

        static void Init();
        static void Update();
        static void Shutdown();

        static bool IsConnected(int index);

        static bool IsButtonDown(int index, GamepadButton button);
        static bool IsButtonPressed(int index, GamepadButton button);
        static bool IsButtonReleased(int index, GamepadButton button);

        static float GetLeftStickX(int index);
        static float GetLeftStickY(int index);
        static float GetRightStickX(int index);
        static float GetRightStickY(int index);
        static float GetLeftTrigger(int index);
        static float GetRightTrigger(int index);

        static void SetRumble(int index, float leftMotor, float rightMotor);
        static void StopRumble(int index);

        static std::string ButtonToString(GamepadButton button);

    private:
        static std::array<GamepadState, MaxGamepads> s_states;
    };
}
