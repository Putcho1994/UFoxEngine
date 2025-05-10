//
// Created by b-boy on 03.05.2025.
//

#include <SDL3/SDL_mouse.h>
#include <glm/glm.hpp>

#include "SDL3/SDL_events.h"
#include <fmt/core.h>

namespace ufox {
    class InputSystem {

        public:

        glm::vec2 getMousePosition() const;


        void updateMousePositionInsideWindow();
        void updateMousePositionOutsideWindow(SDL_Window *window);
        void EnabledMousePositionOutside(bool state);

        private:

        bool mouseIsOutSideWindow{true};
        float previousGlobalMouseX;
        float previousGlobalMouseY;
        float currentGlobalMouseX;
        float currentGlobalMouseY;
        float currentLocalMouseX;
        float currentLocalMouseY;



    };
}