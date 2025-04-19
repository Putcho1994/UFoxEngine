#include <fmt/core.h>
#include <Windowing/ufox_windowing.hpp>
#include <Engine/ufox_graphic.hpp>

int main() {
    try {
        ufox::windowing::sdl::UfoxWindow window("UFoxEngine Test",
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        ufox::graphics::vulkan::GraphicsDevice device(
            "UFox Engine", vk::makeApiVersion(0, 1, 0,0),
            "UFox Application", vk::makeApiVersion(0,1,0,0));


        window.show();

        SDL_Event event;
        bool running = true;
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
            }
            SDL_Delay(16);
        }

        return 0;
    } catch (const ufox::windowing::sdl::SDLException &e) {
        fmt::println("Error: {}", e.what());
        return 1;
    }
}
