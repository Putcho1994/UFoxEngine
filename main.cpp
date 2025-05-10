#include <fmt/core.h>
#include <Windowing/ufox_windowing.hpp>
#include <Engine/ufox_graphic.hpp>
#include <Engine/ufox_inputSystem.hpp>


int main() {
    try {
        ufox::windowing::sdl::UfoxWindow window("UFoxEngine Test",
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        ufox::graphics::vulkan::GraphicsDevice gpu(window,
            "UFox Engine", vk::makeApiVersion(0, 1, 0,0),
            "UFox Application", vk::makeApiVersion(0,1,0,0));
        ufox::InputSystem input{};



        window.show();

        SDL_Event event;
        bool running = true;
        bool c {true};

        auto windowflag = SDL_GetWindowFlags(window.get());
        gpu.enableRender = !(windowflag & SDL_WINDOW_MINIMIZED) && !(windowflag & SDL_WINDOW_HIDDEN);

        while (running) {
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_EVENT_QUIT: {
                        gpu.enableRender = false;
                        running = false;
                        break;
                    }
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                        window.updateSize();
                        auto [w, h] = window.getSize();
                        fmt::println("Window resized: {}x{}", w, h);
                        gpu.recreateSwapchain(window);
                        break;
                    }
                    case SDL_EVENT_WINDOW_MINIMIZED: {
                        gpu.enableRender = false;
                        break;
                    }
                    case SDL_EVENT_WINDOW_RESTORED: {
                        gpu.enableRender = true;
                        break;
                    }
                    case SDL_EVENT_MOUSE_MOTION: {
                        input.EnabledMousePositionOutside(false);
                        input.updateMousePositionInsideWindow();
                        break;
                    }
                    case SDL_EVENT_WINDOW_FOCUS_LOST: {
                        input.EnabledMousePositionOutside(false);
                        break;
                    }
                    case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                        input.EnabledMousePositionOutside(true);
                        break;
                    }
                    default: {
                        if (event.motion.x == 0 || event.motion.y == 0) {
                            input.EnabledMousePositionOutside(true);
                        }

                    }
                }
            }

            input.updateMousePositionOutsideWindow(window.get());

            gpu.drawFrame(window);
        }

        gpu.waitForIdle();

        return 0;
    } catch (const ufox::windowing::sdl::SDLException &e) {
        fmt::println("Error: {}", e.what());
        return 1;
    }
}
