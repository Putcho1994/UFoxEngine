//
// Created by b-boy on 19.04.2025.
//

#pragma once

#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <fmt/base.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>

#include "Windowing/ufox_windowing.hpp"

namespace ufox::graphics::vulkan {

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
    };

    static bool AreExtensionsSupported(const std::vector<const char*>& required, const std::vector<vk::ExtensionProperties>& available);

    class GraphicsDevice {
    public:
        GraphicsDevice(const windowing::sdl::UfoxWindow& window, const char* engineName, uint32_t engineVersion, const char* appName, uint32_t appVersion);
        ~GraphicsDevice() = default;

        // Delete copy constructors
        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;

        // Delete move constructors
        GraphicsDevice(GraphicsDevice&&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;

        void createSwapChain(const windowing::sdl::UfoxWindow& window);
        void recreateSwapchain();
        void clearSwapchain();

        bool useVsync{true};

    private:
        // RAII Vulkan context
        std::optional<vk::raii::Context> context{};
        std::optional<vk::raii::Instance> instance{};
        std::optional<vk::raii::SurfaceKHR> surface{};
        std::optional <vk::raii::PhysicalDevice> physicalDevice{};
        std::optional <vk::raii::Device> device{};
        QueueFamilyIndices queueFamilyIndices{ std::nullopt, std::nullopt };
        std::optional<vk::raii::Queue> graphicsQueue{};
        std::optional<vk::raii::Queue> presentQueue{};
        std::optional<vk::raii::CommandPool> commandPool{};
        std::optional<vk::raii::SwapchainKHR> swapchain{};
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::raii::ImageView> swapchainImageViews;
        vk::Format swapchainFormat{ vk::Format::eUndefined };
        vk::PresentModeKHR presentMode{ vk::PresentModeKHR::eFifo};
        vk::Extent2D swapchainExtent{ 0, 0 };
    };

}
