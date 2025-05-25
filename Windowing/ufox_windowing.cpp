//
// Created by b-boy on 19.04.2025.
//

#include "ufox_windowing.hpp"

#include "fmt/base.h"

namespace ufox::windowing::sdl
{
    SDLException::SDLException(const std::string& msg) : std::runtime_error(msg + ": " + SDL_GetError()) {}

    UfoxWindow::UfoxWindow(const std::string& title, Uint32 flags):_window{nullptr, SDL_DestroyWindow} {
        if (!SDL_Init(SDL_INIT_VIDEO)) throw SDLException("Failed to initialize SDL");

        SDL_DisplayID primary = SDL_GetPrimaryDisplay();
        SDL_Rect usableBounds{};
        SDL_GetDisplayUsableBounds(primary, &usableBounds);

        // Create window while ensuring proper cleanup on failure
        // SDL_Window *rawWindow = SDL_CreateWindow(title.c_str(),
        //     usableBounds.w - 4, usableBounds.h - 34, flags);

        SDL_Window *rawWindow = SDL_CreateWindow(title.c_str(),
            800, 600, flags);

        if (!rawWindow) {
            throw SDLException("Failed to create window");
        }

        _window.reset(rawWindow);

        SDL_SetWindowPosition(_window.get(), 50, 50);


        if (flags & SDL_WINDOW_VULKAN) {
            if (!SDL_Vulkan_LoadLibrary(nullptr)) throw SDLException("Failed to load Vulkan library");
        }

        int w, h;
        SDL_GetWindowPosition(_window.get(), &w, &h);
        _position = glm::vec2(static_cast<float>(w),static_cast<float>(h));

        updateSize();

        _topLeftResizeHandle.transform.x = _position.x -10;
        _topLeftResizeHandle.transform.y = _position.y;
        _topLeftResizeHandle.transform.width = 10;
        _topLeftResizeHandle.transform.height = 25;
    }

    SDL_Window* UfoxWindow::get() const {
        return _window.get();
    }

    void UfoxWindow::show() const {
        SDL_ShowWindow(_window.get());
    }

    void UfoxWindow::hide() const {
        SDL_HideWindow(_window.get());
    }

    void UfoxWindow::updateSize(){
        int w, h;
        SDL_GetWindowSizeInPixels(_window.get(), &w, &h);
        _size = glm::vec2(static_cast<float>(w),static_cast<float>(h));
        int x, y;
        SDL_GetWindowPosition(_window.get(), &x, &y);
        _position = glm::vec2(static_cast<float>(x),static_cast<float>(y));
    }

    void UfoxWindow::onUpdate(const InputSystem &inputSystem) {




    }
}

