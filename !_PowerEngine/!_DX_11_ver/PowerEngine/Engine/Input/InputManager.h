#pragma once
#include <array>
#include <string>
#include <SDL2/SDL.h>

namespace Engine
{
    enum class Key
    {
        A = SDL_SCANCODE_A, B = SDL_SCANCODE_B, C = SDL_SCANCODE_C,
        D = SDL_SCANCODE_D, E = SDL_SCANCODE_E, F = SDL_SCANCODE_F,
        G = SDL_SCANCODE_G, H = SDL_SCANCODE_H, I = SDL_SCANCODE_I,
        J = SDL_SCANCODE_J, K = SDL_SCANCODE_K, L = SDL_SCANCODE_L,
        M = SDL_SCANCODE_M, N = SDL_SCANCODE_N, O = SDL_SCANCODE_O,
        P = SDL_SCANCODE_P, Q = SDL_SCANCODE_Q, R = SDL_SCANCODE_R,
        S = SDL_SCANCODE_S, T = SDL_SCANCODE_T, U = SDL_SCANCODE_U,
        V = SDL_SCANCODE_V, W = SDL_SCANCODE_W, X = SDL_SCANCODE_X,
        Y = SDL_SCANCODE_Y, Z = SDL_SCANCODE_Z,

        Num0 = SDL_SCANCODE_0, Num1 = SDL_SCANCODE_1, Num2 = SDL_SCANCODE_2,
        Num3 = SDL_SCANCODE_3, Num4 = SDL_SCANCODE_4, Num5 = SDL_SCANCODE_5,
        Num6 = SDL_SCANCODE_6, Num7 = SDL_SCANCODE_7, Num8 = SDL_SCANCODE_8,
        Num9 = SDL_SCANCODE_9,

        F1 = SDL_SCANCODE_F1, F2 = SDL_SCANCODE_F2, F3 = SDL_SCANCODE_F3,
        F4 = SDL_SCANCODE_F4, F5 = SDL_SCANCODE_F5, F6 = SDL_SCANCODE_F6,
        F7 = SDL_SCANCODE_F7, F8 = SDL_SCANCODE_F8, F9 = SDL_SCANCODE_F9,
        F10 = SDL_SCANCODE_F10, F11 = SDL_SCANCODE_F11, F12 = SDL_SCANCODE_F12,

        Space = SDL_SCANCODE_SPACE,
        Enter = SDL_SCANCODE_RETURN,
        Escape = SDL_SCANCODE_ESCAPE,
        Tab = SDL_SCANCODE_TAB,
        Backspace = SDL_SCANCODE_BACKSPACE,
        Delete = SDL_SCANCODE_DELETE,
        Insert = SDL_SCANCODE_INSERT,

        Home = SDL_SCANCODE_HOME,
        End = SDL_SCANCODE_END,
        PageUp = SDL_SCANCODE_PAGEUP,
        PageDown = SDL_SCANCODE_PAGEDOWN,

        CapsLock = SDL_SCANCODE_CAPSLOCK,
        ScrollLock = SDL_SCANCODE_SCROLLLOCK,
        NumLock = SDL_SCANCODE_NUMLOCKCLEAR,

        PrintScreen = SDL_SCANCODE_PRINTSCREEN,
        Pause = SDL_SCANCODE_PAUSE,

        LShift = SDL_SCANCODE_LSHIFT, RShift = SDL_SCANCODE_RSHIFT,
        LCtrl = SDL_SCANCODE_LCTRL, RCtrl = SDL_SCANCODE_RCTRL,
        LAlt = SDL_SCANCODE_LALT, RAlt = SDL_SCANCODE_RALT,

        Up = SDL_SCANCODE_UP, Down = SDL_SCANCODE_DOWN,
        Left = SDL_SCANCODE_LEFT, Right = SDL_SCANCODE_RIGHT,

        KP0 = SDL_SCANCODE_KP_0, KP1 = SDL_SCANCODE_KP_1,
        KP2 = SDL_SCANCODE_KP_2, KP3 = SDL_SCANCODE_KP_3,
        KP4 = SDL_SCANCODE_KP_4, KP5 = SDL_SCANCODE_KP_5,
        KP6 = SDL_SCANCODE_KP_6, KP7 = SDL_SCANCODE_KP_7,
        KP8 = SDL_SCANCODE_KP_8, KP9 = SDL_SCANCODE_KP_9,

        KPEnter = SDL_SCANCODE_KP_ENTER,
        KPPlus = SDL_SCANCODE_KP_PLUS,
        KPMinus = SDL_SCANCODE_KP_MINUS,
        KPMultiply = SDL_SCANCODE_KP_MULTIPLY,
        KPDivide = SDL_SCANCODE_KP_DIVIDE,
        KPDecimal = SDL_SCANCODE_KP_DECIMAL,

        GraveAccent = SDL_SCANCODE_GRAVE,
        Minus = SDL_SCANCODE_MINUS,
        Equals = SDL_SCANCODE_EQUALS,
        LeftBracket = SDL_SCANCODE_LEFTBRACKET,
        RightBracket = SDL_SCANCODE_RIGHTBRACKET,
        Backslash = SDL_SCANCODE_BACKSLASH,
        Semicolon = SDL_SCANCODE_SEMICOLON,
        Apostrophe = SDL_SCANCODE_APOSTROPHE,
        Comma = SDL_SCANCODE_COMMA,
        Period = SDL_SCANCODE_PERIOD,
        Slash = SDL_SCANCODE_SLASH,

        Count = SDL_NUM_SCANCODES
    };

    enum class MouseButton
    {
        Left = 0,
        Middle = 1,
        Right = 2,
        X1 = 3,
        X2 = 4,
        Count = 5
    };

    class InputManager
    {
    public:
        static void Init();
        static void Update();
        static void ProcessEvent(const SDL_Event& event);
        static void Shutdown();

        static bool IsKeyDown(Key key);
        static bool IsKeyPressed(Key key);
        static bool IsKeyReleased(Key key);

        static bool IsMouseButtonDown(MouseButton button);
        static bool IsMouseButtonPressed(MouseButton button);
        static bool IsMouseButtonReleased(MouseButton button);

        static float GetMouseX();
        static float GetMouseY();
        static float GetMouseDeltaX();
        static float GetMouseDeltaY();
        static float GetMouseScrollDelta();

        static Key         GetKeyDown();
        static MouseButton GetMouseButtonDown();
        static std::string KeyToString(Key key);
    };
}
