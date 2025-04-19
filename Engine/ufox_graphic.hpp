//
// Created by b-boy on 19.04.2025.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include <string>
#include <vector>
#include <stdexcept>
#include <SDL3/SDL_vulkan.h>
#include <fmt/base.h>

namespace ufox::graphics::vulkan {

    static bool AreExtensionsSupported(const std::vector<const char*>& required, const std::vector<vk::ExtensionProperties>& available);

    class GraphicsDevice {
    public:
        GraphicsDevice(const char* engineName, uint32_t engineVersion, const char* appName, uint32_t appVersion);
        ~GraphicsDevice() = default;

        // Delete copy constructors
        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;

        // Delete move constructors
        GraphicsDevice(GraphicsDevice&&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;



    private:

        void createInstance();

        // RAII Vulkan context
        std::optional<vk::raii::Context> context{};
        std::optional<vk::raii::Instance> instance{};


    };

}
