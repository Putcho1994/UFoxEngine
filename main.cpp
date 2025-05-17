#include <fmt/core.h>
#include <Windowing/ufox_windowing.hpp>
#include <Engine/ufox_graphic.hpp>
#include <Engine/ufox_inputSystem.hpp>
#include <Engine/ufox_gui.hpp>


int main() {
    try {
        ufox::windowing::sdl::UfoxWindow window("UFoxEngine Test",
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        ufox::gpu::vulkan::GraphicsDevice gpu(window,
            "UFox Engine", vk::makeApiVersion(0, 1, 0,0),
            "UFox Application", vk::makeApiVersion(0,1,0,0));
        ufox::InputSystem input{};

        ufox::gui::GUI gui(gpu);

        ufox::gui::GUIStyle params{};
        params.cornerRadius = glm::vec4(10.0f, 10.0f, 10.0f, 10.0f);
        params.borderThickness = glm::vec4(2.0f, 2.0f, 2.0f, 2.0f);
        params.borderTopColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
        params.borderRightColor = glm::vec4(0.0f, 1.0f, 1.0f, 1.0f);
        params.borderBottomColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        params.borderLeftColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        gui.addStyle("default", params, ufox::gpu::vulkan::MAX_FRAMES_IN_FLIGHT);

        gui.init();

        window.show();

        SDL_Event event;
        bool running = true;


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
                        gui.update();
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
                        gui.update();
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

            if (gpu.enableRender) {
                const vk::raii::CommandBuffer& cmd = *gpu.beginFrame(window);
                if (cmd != nullptr) {
                    gpu.beginDynamicRendering(cmd);

                    gui.draw(cmd);

                    gpu.endDynamicRendering(cmd);
                    gpu.endFrame(cmd, window);
                }
            }
        }

        gpu.waitForIdle();

        return 0;
    } catch (const ufox::windowing::sdl::SDLException &e) {
        fmt::println("Error: {}", e.what());
        return 1;
    }
}
