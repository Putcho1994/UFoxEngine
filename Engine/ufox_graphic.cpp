//
// Created by b-boy on 19.04.2025.
//

#include "ufox_graphic.hpp"


namespace ufox::graphics::vulkan {

    bool AreExtensionsSupported( const std::vector<const char *> &required, const std::vector<vk::ExtensionProperties> &available) {
        for (const auto* req : required) {
            bool found = false;
            for (const auto& avail : available) {
                if (strcmp(req, avail.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                fmt::println("Missing extension: {}", req);
                return false;
            }
        }
        return true;
    }

    GraphicsDevice::GraphicsDevice(const windowing::sdl::UfoxWindow& window, const char* engineName, uint32_t engineVersion, const char* appName, uint32_t appVersion){

#pragma region Create Context
        auto vkGetInstanceProcAddr{reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr())};
        context.emplace(vkGetInstanceProcAddr);
        auto const vulkanVersion {context->enumerateInstanceVersion()};
        fmt::println("Vulkan API version: {}.{}", VK_VERSION_MAJOR(vulkanVersion), VK_VERSION_MINOR(vulkanVersion));
#pragma endregion

#pragma region Create Instance
        vk::ApplicationInfo appInfo{};
        appInfo.setPApplicationName(appName)
            .setApplicationVersion(appVersion)
            .setPEngineName(engineName)
            .setEngineVersion(engineVersion)
            .setApiVersion(vulkanVersion);


        std::vector<vk::ExtensionProperties> availableExtensions = context->enumerateInstanceExtensionProperties();
        std::vector<const char*> requiredExtensions{};

        uint32_t extensionCount = 0;
        const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
        if (extensions == nullptr) {
            throw std::runtime_error("Failed to get SDL Vulkan extensions");
        }

        requiredExtensions.insert(requiredExtensions.end(), extensions, extensions + extensionCount);

#if !defined(NDEBUG)
        for (size_t i = 0; i < extensionCount; ++i) {
            fmt::println("SDL extension: {}", extensions[i]);
        }
#endif

        if (!AreExtensionsSupported(requiredExtensions, availableExtensions)) {
            throw std::runtime_error("Required device extensions are missing");
        }

        vk::InstanceCreateInfo createInfo{};
        createInfo.setPApplicationInfo(&appInfo)
            .setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()))
            .setPpEnabledExtensionNames(requiredExtensions.data());

        instance.emplace(*context, createInfo);
#pragma endregion

#pragma region Create Surface
        VkSurfaceKHR raw_surface;
          if (!SDL_Vulkan_CreateSurface(window.get(),**instance, nullptr, &raw_surface)) {
              throw windowing::sdl::SDLException("Failed to create surface");
          }

        surface.emplace(*instance,raw_surface);
#pragma endregion

    }

}

