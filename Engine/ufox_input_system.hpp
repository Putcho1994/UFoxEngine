//
// Created by b-boy on 03.05.2025.
//
#pragma once

#include <memory>
#include <SDL3/SDL_mouse.h>
#include <glm/glm.hpp>

#include <SDL3/SDL_events.h>
#include <fmt/core.h>
#include <vector>

namespace ufox {

    enum class CursorType {
        eDefault,
        eEWResize,

    };

    enum class MouseButtonState {
        eNone,
        eDown,
        eUp,
        eHold,
    };



    class InputSystem {
        public:

        InputSystem() = default;
        ~InputSystem() =default;

        glm::vec2 getMousePosition() const;


        void updateMousePositionInsideWindow();
        void updateMousePositionOutsideWindow(SDL_Window *window);
        void enabledMousePositionOutside(bool state);

        void updateMouseEvents(const SDL_Event& event);

        bool isMouseButtonDown() const{return _isMouseButtonDown;}
        bool isMouseButtonUp() const{return _isMouseButtonUp;}


        SDL_Cursor* getDefaultCursor() const {return _defaultCursor.get();}
        SDL_Cursor* getEWResizeCursor() const {return _ewResizeCursor.get();}

        void setCursor(CursorType type);

        private:

        bool mouseIsOutSideWindow{true};
        float previousGlobalMouseX {0};
        float previousGlobalMouseY {0};
        float currentGlobalMouseX {0};
        float currentGlobalMouseY {0};
        float currentLocalMouseX {0};
        float currentLocalMouseY {0};

        CursorType currentCursor{CursorType::eDefault};

        std::unique_ptr<SDL_Cursor, decltype(&SDL_DestroyCursor)> _defaultCursor ={SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT), SDL_DestroyCursor};
        std::unique_ptr<SDL_Cursor, decltype(&SDL_DestroyCursor)> _ewResizeCursor ={SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE), SDL_DestroyCursor};

        bool _isMouseButtonDown{false};
        bool _isMouseButtonUp{false};

    };
}