#include <fmt/core.h>
#include <Windowing/ufox_windowing.hpp>
#include <Engine/ufox_graphic.hpp>
#include <Engine/ufox_inputSystem.hpp>
#include <Engine/ufox_gui.hpp>
#include <Engine/ufox_panel.hpp>


int main() {
    try {
        ufox::windowing::sdl::UfoxWindow window("UFoxEngine Test",
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        ufox::gpu::vulkan::GraphicsDevice gpu(window,
            "UFox Engine", vk::makeApiVersion(0, 1, 0,0),
            "UFox Application", vk::makeApiVersion(0,1,0,0));
        ufox::InputSystem input{};

        ufox::gui::GUI gui(gpu);

        ufox::Panel mainPanel{input};
        mainPanel.rootElement.style.backgroundColor = {0.11f, 0.11f, 0.11f, 1.0f};
        mainPanel.transform.x = 0;
        mainPanel.transform.y = 0;


        ufox::Panel sidePanel{input};
        sidePanel.rootElement.style.backgroundColor = {0.5f, 0.5f, 0.5f, 1.0f};
        sidePanel.transform.x = 400; // Right of mainPanel
        sidePanel.transform.y = 0;


        mainPanel.BridgePanel(sidePanel, ufox::AttachmentPosition::eRight);



        gui.elements.push_back(&mainPanel.rootElement);
        gui.elements.push_back(&sidePanel.rootElement);

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

                        gpu.recreateSwapchain(window);
                        mainPanel.onResize(glm::vec2(w, h));
                        sidePanel.onResize(glm::vec2(w, h));
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
                        input.enabledMousePositionOutside(false);
                        input.updateMousePositionInsideWindow();
                        mainPanel.onUpdate(event);
                        sidePanel.onUpdate(event);

                        break;
                    }
                    case SDL_EVENT_WINDOW_FOCUS_LOST: {
                        input.enabledMousePositionOutside(false);
                        break;
                    }
                    case SDL_EVENT_WINDOW_FOCUS_GAINED: {
                        input.enabledMousePositionOutside(true);
                        break;
                    }
                    default: {
                        if (event.motion.x == 0 || event.motion.y == 0) {
                            input.enabledMousePositionOutside(true);
                        }

                    }
                }
            }

            input.updateMousePositionOutsideWindow(window.get());


            if (gpu.enableRender) {
                gui.update();
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
