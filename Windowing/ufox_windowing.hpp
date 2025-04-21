//
// Created by b-boy on 19.04.2025.
//
#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <memory>
#include <string>

namespace ufox::windowing::sdl {

    class SDLException : public std::runtime_error {
    public:
        explicit SDLException(const std::string &msg);
    };

    class UfoxWindow {
    public:
        UfoxWindow(const std::string& title, Uint32 flags);
        ~UfoxWindow() = default;

        // Delete copy operations
        UfoxWindow(const UfoxWindow&) = delete;
        UfoxWindow& operator=(const UfoxWindow&) = delete;

        // Allow move operations
        UfoxWindow(UfoxWindow&&) = default;
        UfoxWindow& operator=(UfoxWindow&&) = default;

        [[nodiscard]] SDL_Window* get() const;
        [[nodiscard]] std::pair<uint32_t, uint32_t> getSize() const{return _size;}

        void show() const;
        void hide() const;
        void updateSize();

    private:
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window;
        std::pair<uint32_t, uint32_t> _size;
    };
}
