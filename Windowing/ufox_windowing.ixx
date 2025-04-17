//
// Created by b-boy on 13.04.2025.
//
module;

#include <memory>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

export module ufox_windowing;
// import declaration

import fmt;
import vulkan_hpp;
import ufoxUtils;

export namespace ufox::windowing {
    class SDLException : public std::runtime_error {
    public:
        explicit SDLException(const std::string &msg) : std::runtime_error(msg + ": " + SDL_GetError()) {
        }
    };

    class UfoxWindow {
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{nullptr, SDL_DestroyWindow};

        PFN_vkGetInstanceProcAddr getInstanceProcAddr = nullptr;

    public:
        void Init(const char *title, Uint32 flags) {

#ifdef UFOX_DEBUG
            utils::logger::BeginDebugBlog("WINDOWING");
#endif

            if (!SDL_Init(SDL_INIT_VIDEO)) throw SDLException("Failed to initialize SDL");

            SDL_DisplayID primary = SDL_GetPrimaryDisplay();
            SDL_Rect usableBounds{};
            SDL_GetDisplayUsableBounds(primary, &usableBounds);

            // Create window while ensuring proper cleanup on failure
            SDL_Window *rawWindow = SDL_CreateWindow(
                title,
                usableBounds.w - 4, usableBounds.h - 34, // Title bar & resize handle
                flags
            );

            if (!rawWindow) {
                throw SDLException("Failed to create window");
            }

            window.reset(rawWindow);

            SDL_SetWindowPosition(window.get(), 2, 32);

#ifdef UFOX_DEBUG
            utils::logger::log_debug("Create SDL Window", "success");
#endif

            if (flags & SDL_WINDOW_VULKAN) {
                if (!SDL_Vulkan_LoadLibrary(nullptr)) throw SDLException("Failed to load Vulkan library");

#ifdef UFOX_DEBUG
                utils::logger::log_debug("Load Vulkan Library", "success");
#endif

                auto instanceProcAddr = SDL_Vulkan_GetVkGetInstanceProcAddr();
                if (!instanceProcAddr) {
                    throw SDLException("Failed to get vkGetInstanceProcAddr");
                }
                getInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(instanceProcAddr);

#ifdef UFOX_DEBUG
                utils::logger::log_debug("Get InstanceProcAddr", "success");
                utils::logger::log_debug("Windowing Init", "complete");
                utils::logger::EndDebugBlog();
#endif

            }
        }

        [[nodiscard]] PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const {
            return getInstanceProcAddr;
        }

        void CreateSurface(const vk::raii::Instance &vkInstance, std::optional<vk::raii::SurfaceKHR> &vkSurface) const {
            VkSurfaceKHR surface;
            if (!SDL_Vulkan_CreateSurface(window.get(), *vkInstance, nullptr, &surface)) {
                throw SDLException("Failed to create surface");
            }

            vkSurface.emplace(vkInstance, surface);
        }

        [[nodiscard]] bool ShowWindow() const { return SDL_ShowWindow(window.get()); }
        void HideWindow() const { SDL_HideWindow(window.get()); }
        void CloseWindow() const { SDL_DestroyWindow(window.get()); }
    };
}
