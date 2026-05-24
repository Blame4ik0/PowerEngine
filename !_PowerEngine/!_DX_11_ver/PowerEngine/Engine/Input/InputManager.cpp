#include "InputManager.h"
#include "Core/Logger.h"
#include <string>

namespace Engine
{
    static std::array<bool, static_cast<size_t>(Key::Count)> s_currentKeys{};
    static std::array<bool, static_cast<size_t>(Key::Count)> s_previousKeys{};

    static std::array<bool, static_cast<size_t>(MouseButton::Count)> s_currentMouse{};
    static std::array<bool, static_cast<size_t>(MouseButton::Count)> s_previousMouse{};

    static float s_mouseX = 0.0f;
    static float s_mouseY = 0.0f;
    static float s_prevMouseX = 0.0f;
    static float s_prevMouseY = 0.0f;
    static float s_scrollDelta = 0.0f;

    void InputManager::Init()
    {
        s_currentKeys.fill(false);
        s_previousKeys.fill(false);
        s_currentMouse.fill(false);
        s_previousMouse.fill(false);
        LOG_INFO("InputManager initialized.");
    }

    void InputManager::Shutdown()
    {
        LOG_INFO("InputManager shut down.");
    }

    void InputManager::Update()
    {
        s_previousKeys = s_currentKeys;
        s_previousMouse = s_currentMouse;

        const Uint8* sdlKeys = SDL_GetKeyboardState(nullptr);
        for (int i = 0; i < static_cast<int>(Key::Count); i++)
            s_currentKeys[i] = sdlKeys[i] != 0;

        s_prevMouseX = s_mouseX;
        s_prevMouseY = s_mouseY;
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        s_mouseX = static_cast<float>(mx);
        s_mouseY = static_cast<float>(my);

        s_scrollDelta = 0.0f;
    }

    void InputManager::ProcessEvent(const SDL_Event& event)
    {
        switch (event.type)
        {
        case SDL_MOUSEBUTTONDOWN:
        {
            int idx = event.button.button - 1;
            if (idx >= 0 && idx < static_cast<int>(MouseButton::Count))
                s_currentMouse[idx] = true;
            break;
        }
        case SDL_MOUSEBUTTONUP:
        {
            int idx = event.button.button - 1;
            if (idx >= 0 && idx < static_cast<int>(MouseButton::Count))
                s_currentMouse[idx] = false;
            break;
        }
        case SDL_MOUSEWHEEL:
            s_scrollDelta = static_cast<float>(event.wheel.y);
            break;
        }
    }

    bool InputManager::IsKeyDown(Key key)
    {
        return s_currentKeys[static_cast<size_t>(key)];
    }

    bool InputManager::IsKeyPressed(Key key)
    {
        size_t idx = static_cast<size_t>(key);
        return s_currentKeys[idx] && !s_previousKeys[idx];
    }

    bool InputManager::IsKeyReleased(Key key)
    {
        size_t idx = static_cast<size_t>(key);
        return !s_currentKeys[idx] && s_previousKeys[idx];
    }

    bool InputManager::IsMouseButtonDown(MouseButton button)
    {
        return s_currentMouse[static_cast<size_t>(button)];
    }

    bool InputManager::IsMouseButtonPressed(MouseButton button)
    {
        size_t idx = static_cast<size_t>(button);
        return s_currentMouse[idx] && !s_previousMouse[idx];
    }

    bool InputManager::IsMouseButtonReleased(MouseButton button)
    {
        size_t idx = static_cast<size_t>(button);
        return !s_currentMouse[idx] && s_previousMouse[idx];
    }

    float InputManager::GetMouseX() { return s_mouseX; }
    float InputManager::GetMouseY() { return s_mouseY; }
    float InputManager::GetMouseDeltaX() { return s_mouseX - s_prevMouseX; }
    float InputManager::GetMouseDeltaY() { return s_mouseY - s_prevMouseY; }
    float InputManager::GetMouseScrollDelta() { return s_scrollDelta; }

    Key InputManager::GetKeyDown()
    {
        for (int i = 0; i < static_cast<int>(Key::Count); i++)
            if (s_currentKeys[i] && !s_previousKeys[i])
                return static_cast<Key>(i);
        return static_cast<Key>(-1);
    }

    MouseButton InputManager::GetMouseButtonDown()
    {
        for (int i = 0; i < static_cast<int>(MouseButton::Count); i++)
            if (s_currentMouse[i] && !s_previousMouse[i])
                return static_cast<MouseButton>(i);
        return static_cast<MouseButton>(-1);
    }

    std::string InputManager::KeyToString(Key key)
    {
        switch (key)
        {
        case Key::A: return "A"; case Key::B: return "B"; case Key::C: return "C";
        case Key::D: return "D"; case Key::E: return "E"; case Key::F: return "F";
        case Key::G: return "G"; case Key::H: return "H"; case Key::I: return "I";
        case Key::J: return "J"; case Key::K: return "K"; case Key::L: return "L";
        case Key::M: return "M"; case Key::N: return "N"; case Key::O: return "O";
        case Key::P: return "P"; case Key::Q: return "Q"; case Key::R: return "R";
        case Key::S: return "S"; case Key::T: return "T"; case Key::U: return "U";
        case Key::V: return "V"; case Key::W: return "W"; case Key::X: return "X";
        case Key::Y: return "Y"; case Key::Z: return "Z";

        case Key::Num0: return "0"; case Key::Num1: return "1";
        case Key::Num2: return "2"; case Key::Num3: return "3";
        case Key::Num4: return "4"; case Key::Num5: return "5";
        case Key::Num6: return "6"; case Key::Num7: return "7";
        case Key::Num8: return "8"; case Key::Num9: return "9";

        case Key::F1:  return "F1";  case Key::F2:  return "F2";
        case Key::F3:  return "F3";  case Key::F4:  return "F4";
        case Key::F5:  return "F5";  case Key::F6:  return "F6";
        case Key::F7:  return "F7";  case Key::F8:  return "F8";
        case Key::F9:  return "F9";  case Key::F10: return "F10";
        case Key::F11: return "F11"; case Key::F12: return "F12";

        case Key::Space:     return "Space";
        case Key::Enter:     return "Enter";
        case Key::Escape:    return "Escape";
        case Key::Tab:       return "Tab";
        case Key::Backspace: return "Backspace";
        case Key::Delete:    return "Delete";
        case Key::Insert:    return "Insert";
        case Key::Home:      return "Home";
        case Key::End:       return "End";
        case Key::PageUp:    return "PageUp";
        case Key::PageDown:  return "PageDown";

        case Key::CapsLock:   return "CapsLock";
        case Key::ScrollLock: return "ScrollLock";
        case Key::NumLock:    return "NumLock";
        case Key::PrintScreen: return "PrintScreen";
        case Key::Pause:      return "Pause";

        case Key::LShift: return "LShift"; case Key::RShift: return "RShift";
        case Key::LCtrl:  return "LCtrl";  case Key::RCtrl:  return "RCtrl";
        case Key::LAlt:   return "LAlt";   case Key::RAlt:   return "RAlt";

        case Key::Up:    return "Up";    case Key::Down:  return "Down";
        case Key::Left:  return "Left";  case Key::Right: return "Right";

        case Key::KP0: return "KP0"; case Key::KP1: return "KP1";
        case Key::KP2: return "KP2"; case Key::KP3: return "KP3";
        case Key::KP4: return "KP4"; case Key::KP5: return "KP5";
        case Key::KP6: return "KP6"; case Key::KP7: return "KP7";
        case Key::KP8: return "KP8"; case Key::KP9: return "KP9";

        case Key::KPEnter:    return "KP Enter";
        case Key::KPPlus:     return "KP +";
        case Key::KPMinus:    return "KP -";
        case Key::KPMultiply: return "KP *";
        case Key::KPDivide:   return "KP /";
        case Key::KPDecimal:  return "KP .";

        case Key::GraveAccent:  return "`";
        case Key::Minus:        return "-";
        case Key::Equals:       return "=";
        case Key::LeftBracket:  return "[";
        case Key::RightBracket: return "]";
        case Key::Backslash:    return "\\";
        case Key::Semicolon:    return ";";
        case Key::Apostrophe:   return "'";
        case Key::Comma:        return ",";
        case Key::Period:       return ".";
        case Key::Slash:        return "/";

        default: return "Unknown";
        }
    }
}
