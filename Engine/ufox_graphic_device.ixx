//
// Created by Putcho on 17.04.2025.
//

module;

#include <optional>



export module ufox_graphic_device;

import ufox_windowing;
import vulkan_hpp;
import ufox_utils;

export namespace ufox::graphic_device::vulkan {
class UfoxGraphicDevice {

    std::optional<vk::raii::Context> context{};
    std::optional<vk::raii::Instance> instance{};
    std::optional<vk::raii::SurfaceKHR> surface{};
    std::optional<vk::raii::PhysicalDevice> physicalDevice{};
    std::optional<vk::raii::Device> device{};

    const windowing::UfoxWindow& _window;

public:
    UfoxGraphicDevice(windowing::UfoxWindow &window): _window(window) {}

    ~UfoxGraphicDevice() = default;

    void Init()
    {
        utils::logger::BeginDebugBlog("[INIT GRAPHIC DEVICE] (VULKAN)");
        auto procAddr = _window.GetVkGetInstanceProcAddr();
        context.emplace(procAddr);
        utils::logger::log_debug("Create Context", "success");

        utils::logger::EndDebugBlog();
    }

};


}