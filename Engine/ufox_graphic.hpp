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
#include <SDL3_image/SDL_image.h>
#include <vulkan/vulkan_raii.hpp>
#include <fstream>
#include "Windowing/ufox_windowing.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>


namespace ufox::gpu {

    struct Vertex {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 texCoord;
    };

    static constexpr Vertex TestRect[] = {
        {{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // Top-left
         {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // Top-right
         {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // Bottom-right
         {{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // Bottom-left

    };

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };


}

namespace ufox::gpu::vulkan {

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    static uint32_t FindMemoryType(const vk::PhysicalDeviceMemoryProperties & memoryProperties, uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask );

    static void TransitionImageLayout(const vk::raii::CommandBuffer& cmd, const vk::Image& image, const vk::Format& format,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
            vk::AccessFlags2 srcAccess, vk::AccessFlags2 dstAccess,
            vk::PipelineStageFlags2 srcStage, vk::PipelineStageFlags2 dstStage);

    static std::vector<char> loadShader(const std::string& filename);

    struct Image {
        std::optional<vk::raii::Image> data{};
        std::optional<vk::raii::DeviceMemory> memory{};
        std::optional<vk::raii::ImageView> view{};
        vk::Format format{ vk::Format::eUndefined};
        vk::Extent2D extent{ 0, 0 };

        void clear() {
            view.reset();
            data.reset();
            memory.reset();
        }
    };

    struct Buffer {
        std::optional<vk::raii::Buffer> data{};
        std::optional<vk::raii::DeviceMemory> memory{};
        uint8_t* mapped;

    };



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

        bool useVsync{false};
        bool enableRender{true};

        void recreateSwapchain(const windowing::sdl::UfoxWindow& window);
        void waitForIdle() const;

        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, Buffer& buffer);
        void createImage(vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                          vk::MemoryPropertyFlags properties, Image& image);
        void copyBuffer(const Buffer& srcBuffer, const Buffer& dstBuffer, const vk::DeviceSize& size) const;
        [[nodiscard]] vk::raii::CommandBuffer beginSingleTimeCommands() const;
        void endSingleTimeCommands(const vk::raii::CommandBuffer& cmd) const;
        void transitionImageLayout(const Image& image,vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const;
        void copyBufferToImage(const Buffer &buffer, const Image &image) const;
        [[nodiscard]] vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;

        const vk::raii::CommandBuffer* beginFrame(const windowing::sdl::UfoxWindow &window);
        void beginDynamicRendering(const vk::raii::CommandBuffer& cmd) const;
        void endDynamicRendering(const vk::raii::CommandBuffer& cmd) const;
        void endFrame(const vk::raii::CommandBuffer& cmd, const windowing::sdl::UfoxWindow &window) ;

        void changeBackgroundColor(const vk::raii::CommandBuffer& cmd,const glm::vec4& color) const;

        [[nodiscard]] const vk::raii::Device& getDevice() const{return *device;}
        [[nodiscard]] const vk::raii::PhysicalDevice& getPhysicalDevice() const{return *physicalDevice;}
        [[nodiscard]] const vk::Extent2D& getSwapchainExtent() const { return swapchainExtent; }
        [[nodiscard]] const vk::Format& getSwapchainFormat() const { return swapchainFormat; }
        [[nodiscard]] const vk::Format& getDepthFormat() const { return depthImage.format; }

        [[nodiscard]] const uint32_t& getCurrentFrame() const { return currentFrame; }
        [[nodiscard]] const uint32_t& getCurrentImage() const { return currentImage; }

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

        Image depthImage{};


        std::optional<vk::raii::DescriptorPool> descriptorPool{};
        std::vector<vk::raii::DescriptorSet> descriptorSets;

        uint32_t currentFrame{ 0 };
        uint32_t currentImage{ 0 };


        Image textureImage{};
        std::optional<vk::raii::Sampler> textureSampler{};


        bool isFrameStarted{false};

        void createSwapchain(const windowing::sdl::UfoxWindow& window);
        void createDepthImage();



    };

}


