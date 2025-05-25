#include <fmt/core.h>
#include <Windowing/ufox_windowing.hpp>
#include <Engine/ufox_graphic.hpp>
#include <Engine/ufox_input_system.hpp>
#include <Engine/ufox_gui_renderer.hpp>
#include <Engine/ufox_view_panel.hpp>




int main() {
    try {
        ufox::windowing::sdl::UfoxWindow window("UFoxEngine Test",
            SDL_WINDOW_VULKAN| SDL_WINDOW_RESIZABLE | SDL_WINDOW_TRANSPARENT);
        ufox::gpu::vulkan::GraphicsDevice gpu(window,
            "UFox Engine", vk::makeApiVersion(0, 1, 0,0),
            "UFox Application", vk::makeApiVersion(0,1,0,0));
        ufox::InputSystem input{};

        ufox::gui::GUI gui(gpu);

        ufox::ViewPanel titleBar{input};
        titleBar.rootElement.style.backgroundColor = {0.14f, 0.15f, 0.16f, 1.0f};
        titleBar.rootElement.style.cornerRadius = glm::vec4{7};
        titleBar.rootElement.style.borderThickness = glm::vec4{1};
        titleBar.rootElement.style.margin = glm::vec4{10};
        titleBar.rootElement.style.borderTopColor = {0.2f, 0.2f, 0.2f, 1.0f};
        titleBar.rootElement.style.borderBottomColor = {0.2f, 0.2f, 0.2f, 1.0f};
        titleBar.rootElement.style.borderLeftColor = {0.2f, 0.2f, 0.2f, 1.0f};
        titleBar.rootElement.style.borderRightColor = {0.2f, 0.2f, 0.2f, 1.0f};
        titleBar.transform.x = 0;
        titleBar.transform.y = 0;
        //
        // ufox::ViewPanel mainPanel{input};
        // mainPanel.rootElement.style.backgroundColor = {0.11f, 0.11f, 0.11f, 1.0f};
        // mainPanel.transform.x = 0;
        // mainPanel.transform.y = 50;
        //
        //
        // ufox::ViewPanel sidePanel{input};
        // sidePanel.rootElement.style.backgroundColor = {0.5f, 0.5f, 0.5f, 1.0f};
        // sidePanel.transform.x = 400; // Right of mainPanel
        // sidePanel.transform.y = 50;


        // mainPanel.BridgePanel(sidePanel, ufox::AttachmentPosition::eRight);
        // titleBar.BridgePanel(mainPanel, ufox::AttachmentPosition::eBottom);
        // titleBar.BridgePanel(sidePanel, ufox::AttachmentPosition::eRight);
        //
        //
        //
        gui.elements.push_back(&titleBar.rootElement);
        // gui.elements.push_back(&mainPanel.rootElement);
        // gui.elements.push_back(&sidePanel.rootElement);

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
                        auto size = window.getSize();

                        gpu.recreateSwapchain(window);
                        titleBar.onResize(size);
                        // mainPanel.onResize(size);
                        // sidePanel.onResize(size);
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
                    case SDL_EVENT_KEY_DOWN: {
                    if (event.key.key == SDLK_SPACE) {
                        fmt::println("Space");
                        SDL_MaximizeWindow(window.get());
                        gpu.recreateSwapchain(window);
                        auto size = window.getSize();
                        titleBar.onResize(size);
                    }

                        break;
                    }

                    default: {}
                }
                input.updateMouseEvents(event);

                titleBar.onUpdate();
                // mainPanel.onUpdate();
                // sidePanel.onUpdate();
            }

            input.updateMousePositionOutsideWindow(window.get());

            window.onUpdate(input);

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
