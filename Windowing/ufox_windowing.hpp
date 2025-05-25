//
// Created by b-boy on 19.04.2025.
//
#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_mouse.h>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include <Engine/ufox_engine_core.hpp>
#include <Engine/ufox_input_system.hpp>


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
        [[nodiscard]] glm::vec2 getPosition() const{return _position;}
        [[nodiscard]] glm::vec2 getSize() const{return _size;}

        void show() const;
        void hide() const;
        void updateSize();
        void onUpdate(const InputSystem& inputSystem);

    private:
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> _window;
        glm::vec2 _position;
        glm::vec2 _size;


        core::DraggerHandle _topLeftResizeHandle{ core::DraggerDirection::eBoth};
        core::DraggerHandle _topRightResizeHandle{ core::DraggerDirection::eBoth};
        core::DraggerHandle _bottomRightResizeHandle{ core::DraggerDirection::eBoth};
        core::DraggerHandle _bottomLeftResizeHandle{ core::DraggerDirection::eBoth};
        core::DraggerHandle _topResizeHandle{ core::DraggerDirection::eVertical};
        core::DraggerHandle _rightResizeHandle{ core::DraggerDirection::eHorizontal};
        core::DraggerHandle _bottomResizeHandle{ core::DraggerDirection::eVertical};
        core::DraggerHandle _leftResizeHandle{ core::DraggerDirection::eHorizontal};

    };
}
