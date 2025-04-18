//
// Created by Putcho on 13.04.2025.
//
module;

#include <SDL3/SDL.h> // Include SDL's traditional header

#include "cmake-build-mingw64-clang-default/_deps/sdl3-src/src/video/khronos/vulkan/vulkan_core.h"


import vulkan_hpp;
import ufox_windowing;


import ufox_utils;
import ufox_graphic_device;

export module main; // Declare the module first

export int main() {
    ufox::utils::logger::restart_timer();

    ufox::windowing::SDL::UfoxWindow window{};
    window.Init("UFox Engine", SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN |SDL_WINDOW_MAXIMIZED);

    ufox::graphic_device::vulkan::UfoxGraphicDevice graphicDevice(window);
    graphicDevice.Init("UFox Engine", "UFox Engine", vk::makeApiVersion(1,0,0,0), VK_API_VERSION_1_3);

    bool mainWinIsShow = window.ShowWindow();

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
