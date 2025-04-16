module;
//
// Created by b-boy on 13.04.2025.
//

#include <SDL3/SDL.h> // Include SDL's traditional header
#include <optional>


#include <vulkan/vulkan_hpp_macros.hpp>
import vulkan_hpp;
import ufox_windowing;
#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif



export module main; // Declare the module first




export int main() {
    std::optional mainWindow = ufox::windowing::SDL::CreateWindow("UFox Engine", SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN |SDL_WINDOW_MAXIMIZED);

    SDL_ShowWindow(mainWindow->get());

    vk::Bool32 hhh = true;
    bool isQuit = false;

    while (!isQuit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                isQuit = true;
            }
        }
    }


    SDL_Quit();

    return 0;
}
