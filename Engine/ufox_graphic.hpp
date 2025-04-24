//
// Created by b-boy on 19.04.2025.
//

#pragma once

#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <fmt/base.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <fstream>
#include <glm/glm.hpp>
#include "Windowing/ufox_windowing.hpp"

#include "ufox_numerics.hpp"

namespace ufox::graphics::geometry {

    struct Vertex {
        numerics::Vector3 position;
        numerics::Vector4 color;
    };

    static const Vertex TestRect[] = {
        {-0.5f, -0.5f, 1.0f, 0.0f, 0.0f},
        {0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
        {0.5f, 0.5f, 0.0f, 0.0f, 1.0f},
        {-0.5f, 0.5f, 1.0f, 1.0f, 1.0f}
    };
}

namespace ufox::graphics::vulkan {

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    static uint32_t FindMemoryType( vk::PhysicalDeviceMemoryProperties const & memoryProperties, uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask );


    static void TransitionImageLayout(const vk::raii::CommandBuffer& cmd, const vk::Image& image,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
            vk::AccessFlags2 srcAccess, vk::AccessFlags2 dstAccess,
            vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage);

    static std::vector<char> loadShader(const std::string& filename);

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

        bool useVsync{true};
        bool enableRender{true};

        void recreateSwapchain(const windowing::sdl::UfoxWindow& window);
        void drawFrame(const windowing::sdl::UfoxWindow& window);
        void waitForIdle() const;

        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, std::optional<vk::raii::Buffer>& buffer, std::optional<vk::raii::DeviceMemory>& bufferMemory);

    private:
        //Instance properties
        std::optional<vk::raii::Context> context{};
        std::optional<vk::raii::Instance> instance{};
        std::optional<vk::raii::SurfaceKHR> surface{};
        std::optional <vk::raii::PhysicalDevice> physicalDevice{};
        std::optional <vk::raii::Device> device{};
        QueueFamilyIndices queueFamilyIndices{ std::nullopt, std::nullopt };
        std::optional<vk::raii::Queue> graphicsQueue{};
        std::optional<vk::raii::Queue> presentQueue{};
        std::optional<vk::raii::CommandPool> commandPool{};
        std::vector<vk::raii::CommandBuffer> commandBuffers;
        std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
        std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
        std::vector<vk::raii::Fence> inFlightFences;

        //Swapchain properties
        std::optional<vk::raii::SwapchainKHR> swapchain{};
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::raii::ImageView> swapchainImageViews;
        vk::Format swapchainFormat{ vk::Format::eUndefined };
        vk::PresentModeKHR presentMode{ vk::PresentModeKHR::eFifo};
        vk::Extent2D swapchainExtent{ 0, 0 };

        std::optional<vk::raii::PipelineLayout> pipelineLayout{};
        std::optional<vk::raii::Pipeline> graphicsPipeline{};

        uint32_t currentFrame{ 0 };
        uint32_t currentImage{ 0 };



        const std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0
        };

        std::optional<vk::raii::Buffer> vertexBuffer{};
        std::optional<vk::raii::DeviceMemory> vertexBufferMemory{};

        std::optional<vk::raii::Buffer> indexBuffer{};
        std::optional<vk::raii::DeviceMemory>indexBufferMemory{};

        void createSwapchain(const windowing::sdl::UfoxWindow& window);
        void createGraphicsPipeline();
        void createVertexBuffer();
        void createIndexBuffer();
    };

}


