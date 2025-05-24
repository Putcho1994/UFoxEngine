//
// Created by b-boy on 03.05.2025.
//

#include "ufox_input_system.hpp"

#include <_mingw_mac.h>


namespace ufox {
    glm::vec2 InputSystem::getMousePosition() const {
        return {currentLocalMouseX, currentLocalMouseY};
    }

    void InputSystem::updateMousePositionInsideWindow() {
        SDL_GetMouseState(&currentLocalMouseX, &currentLocalMouseY);
        //fmt::println("position inside x:{} y:{}", currentLocalMouseX, currentLocalMouseY);
    }

    void InputSystem::updateMousePositionOutsideWindow(SDL_Window *window) {
        if (!mouseIsOutSideWindow) return;
        SDL_GetGlobalMouseState(&currentGlobalMouseX, &currentGlobalMouseY);
        if (currentGlobalMouseX == previousGlobalMouseX && currentGlobalMouseY == previousGlobalMouseY) return;

        int xPos,yPos;
        SDL_GetWindowPosition(window, &xPos, &yPos);
        currentLocalMouseX = currentGlobalMouseX - xPos;
        currentLocalMouseY = currentGlobalMouseY - yPos;
        previousGlobalMouseX = currentGlobalMouseX;
        previousGlobalMouseY = currentGlobalMouseY;
        //fmt::println("position outside x:{} y:{}", currentLocalMouseX, currentLocalMouseY);
    }

    void InputSystem::enabledMousePositionOutside(bool state) {
            if (mouseIsOutSideWindow != state)
                mouseIsOutSideWindow = state;
    }

    void InputSystem::setCursor(const CursorType type) {
        if (currentCursor != type) {
            switch (type) {
                case CursorType::eDefault: {
                    SDL_SetCursor(_defaultCursor.get());
                    break;
                }
                case CursorType::eEWResize: {
                    SDL_SetCursor(_ewResizeCursor.get());
                    break;
                }
                default: {
                    currentCursor = CursorType::eDefault;
                    SDL_SetCursor(_defaultCursor.get());
                    break;
                }
            }

            currentCursor = type;
        }
    }
}
