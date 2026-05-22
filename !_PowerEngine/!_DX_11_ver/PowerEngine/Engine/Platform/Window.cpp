#include "Window.h"
#include "Input/InputManager.h"
#include "Core/Logger.h"
#include <SDL2/SDL_syswm.h>
#include <stdexcept>

namespace Engine
{
    Window::Window(const WindowProps& props)
        : m_props(props)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

        m_window = SDL_CreateWindow(
            m_props.Title.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            m_props.Width, m_props.Height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

        if (!m_window)
            throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

        LOG_INFO("Window created: {}x{} \"{}\"", m_props.Width, m_props.Height, m_props.Title);
    }

    Window::~Window()
    {
        if (m_window)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
        SDL_Quit();
        LOG_INFO("Window destroyed.");
    }

    bool Window::PollEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            InputManager::ProcessEvent(event);

            if (event.type == SDL_QUIT)
                return false;

            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                m_props.Width = event.window.data1;
                m_props.Height = event.window.data2;
                LOG_INFO("Window resized: {}x{}", m_props.Width, m_props.Height);
            }
        }
        return true;
    }

    void* Window::GetHWND() const
    {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (SDL_GetWindowWMInfo(m_window, &info))
            return static_cast<void*>(info.info.win.window);
        return nullptr;
    }
}