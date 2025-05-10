//
// Created by b-boy on 03.05.2025.
//

#include "ufox_inputSystem.hpp"



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

    void InputSystem::EnabledMousePositionOutside(bool state) {
            if (mouseIsOutSideWindow != state)
                mouseIsOutSideWindow = state;
    }

}
