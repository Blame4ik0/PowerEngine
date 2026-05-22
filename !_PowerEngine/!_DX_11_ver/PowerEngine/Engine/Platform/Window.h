#pragma once
#include <string>
#include <SDL2/SDL.h>

namespace Engine
{
    struct WindowProps
    {
        std::string Title = "PowerEngine";
        int         Width = 1280;
        int         Height = 720;
        bool        VSync = true;
        int         RefreshRate = 60;
    };

    class Window
    {
    public:
        explicit Window(const WindowProps& props = {});
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool        PollEvents();

        int         GetWidth()       const { return m_props.Width; }
        int         GetHeight()      const { return m_props.Height; }
        bool        GetVSync()       const { return m_props.VSync; }
        int         GetRefreshRate() const { return m_props.RefreshRate; }
        SDL_Window* GetSDLWindow()   const { return m_window; }
        void* GetHWND()        const;

    private:
        SDL_Window* m_window = nullptr;
        WindowProps m_props;
    };
}