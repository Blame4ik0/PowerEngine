#include "GamepadManager.h"
#include "Core/Logger.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <cmath>

namespace Engine
{
    std::array<GamepadState, GamepadManager::MaxGamepads> GamepadManager::s_states{};

    static constexpr float STICK_DEADZONE = 0.15f;
    static constexpr float TRIGGER_DEADZONE = 0.05f;
    static constexpr float STICK_MAX = 32767.0f;
    static constexpr float TRIGGER_MAX = 255.0f;

    static float ApplyDeadzone(float value, float maxValue, float deadzone)
    {
        if (std::abs(value) < deadzone * maxValue)
            return 0.0f;
        float sign = value > 0.0f ? 1.0f : -1.0f;
        return sign * (std::abs(value / maxValue) - deadzone) / (1.0f - deadzone);
    }

    void GamepadManager::Init()
    {
        for (auto& state : s_states)
            state = GamepadState{};
        LOG_INFO("GamepadManager initialized.");
    }

    void GamepadManager::Shutdown()
    {
        for (int i = 0; i < MaxGamepads; i++)
            StopRumble(i);
        LOG_INFO("GamepadManager shut down.");
    }

    void GamepadManager::Update()
    {
        for (int i = 0; i < MaxGamepads; i++)
        {
            GamepadState& state = s_states[i];
            XINPUT_STATE xstate{};

            bool wasConnected = state.connected;
            state.connected = (XInputGetState(i, &xstate) == ERROR_SUCCESS);

            // Log connect / disconnect events
            if (state.connected && !wasConnected)
                LOG_INFO("Gamepad {} connected.", i);
            else if (!state.connected && wasConnected)
                LOG_WARN("Gamepad {} disconnected.", i);

            if (!state.connected)
            {
                state = GamepadState{};
                continue;
            }

            // Snapshot previous buttons before reading new ones
            state.previousButtons = state.buttons;
            state.buttons = xstate.Gamepad.wButtons;

            // Normalize sticks with radial deadzone
            state.leftStickX = ApplyDeadzone(
                static_cast<float>(xstate.Gamepad.sThumbLX), STICK_MAX, STICK_DEADZONE);
            state.leftStickY = ApplyDeadzone(
                static_cast<float>(xstate.Gamepad.sThumbLY), STICK_MAX, STICK_DEADZONE);
            state.rightStickX = ApplyDeadzone(
                static_cast<float>(xstate.Gamepad.sThumbRX), STICK_MAX, STICK_DEADZONE);
            state.rightStickY = ApplyDeadzone(
                static_cast<float>(xstate.Gamepad.sThumbRY), STICK_MAX, STICK_DEADZONE);

            // Normalize triggers
            state.leftTrigger = ApplyDeadzone(
                static_cast<float>(xstate.Gamepad.bLeftTrigger), TRIGGER_MAX, TRIGGER_DEADZONE);
            state.rightTrigger = ApplyDeadzone(
                static_cast<float>(xstate.Gamepad.bRightTrigger), TRIGGER_MAX, TRIGGER_DEADZONE);
        }
    }

    bool GamepadManager::IsConnected(int index)
    {
        if (index < 0 || index >= MaxGamepads) return false;
        return s_states[index].connected;
    }

    bool GamepadManager::IsButtonDown(int index, GamepadButton button)
    {
        if (!IsConnected(index)) return false;
        return (s_states[index].buttons & static_cast<unsigned short>(button)) != 0;
    }

    bool GamepadManager::IsButtonPressed(int index, GamepadButton button)
    {
        if (!IsConnected(index)) return false;
        auto btn = static_cast<unsigned short>(button);
        return (s_states[index].buttons & btn) != 0 &&
            (s_states[index].previousButtons & btn) == 0;
    }

    bool GamepadManager::IsButtonReleased(int index, GamepadButton button)
    {
        if (!IsConnected(index)) return false;
        auto btn = static_cast<unsigned short>(button);
        return (s_states[index].buttons & btn) == 0 &&
            (s_states[index].previousButtons & btn) != 0;
    }

    float GamepadManager::GetLeftStickX(int index)
    {
        if (!IsConnected(index)) return 0.0f;
        return s_states[index].leftStickX;
    }

    float GamepadManager::GetLeftStickY(int index)
    {
        if (!IsConnected(index)) return 0.0f;
        return s_states[index].leftStickY;
    }

    float GamepadManager::GetRightStickX(int index)
    {
        if (!IsConnected(index)) return 0.0f;
        return s_states[index].rightStickX;
    }

    float GamepadManager::GetRightStickY(int index)
    {
        if (!IsConnected(index)) return 0.0f;
        return s_states[index].rightStickY;
    }

    float GamepadManager::GetLeftTrigger(int index)
    {
        if (!IsConnected(index)) return 0.0f;
        return s_states[index].leftTrigger;
    }

    float GamepadManager::GetRightTrigger(int index)
    {
        if (!IsConnected(index)) return 0.0f;
        return s_states[index].rightTrigger;
    }

    void GamepadManager::SetRumble(int index, float leftMotor, float rightMotor)
    {
        if (!IsConnected(index)) return;

        XINPUT_VIBRATION vibration{};
        vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
        vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
        XInputSetState(index, &vibration);
    }

    void GamepadManager::StopRumble(int index)
    {
        SetRumble(index, 0.0f, 0.0f);
    }

    std::string GamepadManager::ButtonToString(GamepadButton button)
    {
        switch (button)
        {
        case GamepadButton::A:           return "A";
        case GamepadButton::B:           return "B";
        case GamepadButton::X:           return "X";
        case GamepadButton::Y:           return "Y";
        case GamepadButton::DPadUp:      return "DPad Up";
        case GamepadButton::DPadDown:    return "DPad Down";
        case GamepadButton::DPadLeft:    return "DPad Left";
        case GamepadButton::DPadRight:   return "DPad Right";
        case GamepadButton::LeftBumper:  return "LB";
        case GamepadButton::RightBumper: return "RB";
        case GamepadButton::LeftThumb:   return "L3";
        case GamepadButton::RightThumb:  return "R3";
        case GamepadButton::Start:       return "Start";
        case GamepadButton::Back:        return "Back";
        default:                         return "Unknown";
        }
    }
}
