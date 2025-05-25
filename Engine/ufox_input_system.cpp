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

    void InputSystem::updateMouseEvents(const SDL_Event &event) {
        _isMouseButtonDown = false;
        _isMouseButtonUp = false;


        switch (event.type) {
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                _isMouseButtonDown = true;

                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP: {
                _isMouseButtonUp = true;

                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {

                enabledMousePositionOutside(false);
                updateMousePositionInsideWindow();
                break;
            }
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                enabledMousePositionOutside(false);
                break;
            }
            case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                enabledMousePositionOutside(true);
                break;
            }
            default: {
                if (event.motion.x == 0 || event.motion.y == 0) {
                    enabledMousePositionOutside(true);
                }
            }
        }
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
