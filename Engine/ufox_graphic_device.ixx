//
// Created by Putcho on 17.04.2025.
//

module;

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

export module ufox_graphic_device;

import ufox_windowing;
import vulkan_hpp;
import ufox_utils;
import fmt;

static constexpr std::uint32_t API_VERSION = VK_API_VERSION_1_3;
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

    void Init(const char* appTitle, const char* engineName, uint32_t engineVersion, uint32_t apiVersion = API_VERSION) {
#ifdef UFOX_DEBUG
            utils::logger::BeginDebugBlog("[INIT GRAPHIC DEVICE] (VULKAN)");
#endif

            auto procAddr = _window.GetVkGetInstanceProcAddr();
            context.emplace(procAddr);
#ifdef UFOX_DEBUG
            utils::logger::log_debug("Create Context", "success");
#endif

            vk::ApplicationInfo appInfo{};
            appInfo.setPApplicationName(appTitle);
            appInfo.setApplicationVersion(engineVersion);
            appInfo.setPEngineName(engineName);
            appInfo.setEngineVersion(engineVersion);
            appInfo.setApiVersion(apiVersion);





#ifdef UFOX_DEBUG
            utils::logger::EndDebugBlog();
#endif
    }
};
}