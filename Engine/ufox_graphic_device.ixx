//
// Created by Putcho on 17.04.2025.
//

module;

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
#define  VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_hpp_macros.hpp>

export module ufox_graphic_device;

import ufox_windowing;
import vulkan_hpp;
import ufox_utils;
import fmt;


static constexpr std::uint32_t MAX_FRAMES_IN_FLIGHT = 2;

export namespace ufox::graphic_device::vulkan {
class UfoxGraphicDevice {

    std::optional<vk::raii::Context> context{};
    std::optional<vk::raii::Instance> instance{};
    std::optional<vk::raii::SurfaceKHR> surface{};
    std::optional<vk::raii::PhysicalDevice> physicalDevice{};
    std::optional<vk::raii::Device> device{};

    const windowing::SDL::UfoxWindow& _window;

public:
    explicit UfoxGraphicDevice(windowing::SDL::UfoxWindow &window): _window(window) {}

    ~UfoxGraphicDevice() = default;

    void Init(const char* appTitle, const char* engineName, uint32_t engineVersion) {



#ifdef UFOX_DEBUG
            utils::logger::BeginDebugBlog("[INIT GRAPHIC DEVICE] (VULKAN)");
#endif

            auto procAddr = _window.GetVkGetInstanceProcAddr();
            context.emplace(procAddr);
        auto const vulkanVersion {context->enumerateInstanceVersion()};
#ifdef UFOX_DEBUG
            utils::logger::log_debug("Create Context", "success");
#endif

#if ( VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1 )
        // initialize minimal set of function pointers
        VULKAN_HPP_DEFAULT_DISPATCHER.init(procAddr);
#endif

            vk::ApplicationInfo appInfo{};
            appInfo.setPApplicationName(appTitle);
            appInfo.setApplicationVersion(engineVersion);
            appInfo.setPEngineName(engineName);
            appInfo.setEngineVersion(engineVersion);
            appInfo.setApiVersion(vulkanVersion);

        std::vector<vk::ExtensionProperties> availableExtensions = context->enumerateInstanceExtensionProperties();
        std::vector<const char*> requiredExtensions{};

        uint32_t extensionCount = 0;
        const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);


        requiredExtensions.insert(requiredExtensions.end(), extensions, extensions + extensionCount);

        vk::InstanceCreateInfo createInfo{};
        createInfo.setPApplicationInfo(&appInfo)
            .setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()))
            .setPpEnabledExtensionNames(requiredExtensions.data());

        vk::Instance raw_instance;
        vk::createInstance(createInfo,nullptr, &raw_instance);

        instance.emplace(*context, raw_instance);

#if ( VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1 )
        // initialize function pointers for instance
        VULKAN_HPP_DEFAULT_DISPATCHER.init( *instance );
#endif

#ifdef UFOX_DEBUG
            utils::logger::EndDebugBlog();
#endif
    }
};
}