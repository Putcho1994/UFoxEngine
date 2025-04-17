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

import ufoxUtils;


export module main; // Declare the module first

export int main() {
    ufox::utils::logger::restart_timer();

    ufox::windowing::UfoxWindow window{};
    window.Init("UFox Engine", SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN |SDL_WINDOW_MAXIMIZED);



    vk::Bool32 hhh = window.ShowWindow();
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
