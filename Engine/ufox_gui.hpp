//
// Created by b-boy on 15.05.2025.
//

#ifndef UFOX_GUI_HPP
#define UFOX_GUI_HPP
#pragma once
#include <string>
#include <vector>
#include <Engine/ufox_graphic.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <Engine/ufox_engine_core.hpp>

namespace ufox::gui {

    struct GUIVertex {
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 texCoord;
    };

    constexpr GUIVertex Geometry[] {
            {{0.0f, 0.0f,}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}, // Top-left
             {{1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}}, // Top-right
             {{1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // Bottom-right
             {{0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // Bottom-left
    };

    constexpr uint16_t Indices[] {
            0, 1, 2, 2, 3, 0,
    };

    struct GUIElement { // Renamed from Rectangle to GUIElement
        core::TransformRect transform;
        float zIndex;
        uint32_t styleIndex;
    };

    struct GUIStyle {
        glm::vec4 cornerRadius; // Radius for each corner (top-left, top-right, bottom-right, bottom-left)
        glm::vec4 borderThickness; // Thickness for each side (top, right, bottom, left)
        glm::vec4 borderTopColor; // RGBA color for top border
        glm::vec4 borderRightColor; // RGBA color for right border
        glm::vec4 borderBottomColor; // RGBA color for bottom border
        glm::vec4 borderLeftColor; // RGBA color for left border
    };

    struct GUIStyleBuffer {
        std::string name; // Identifier for the style buffer
        GUIStyle content; // Styling properties (corner radius, border thickness, colors)
        std::vector<gpu::vulkan::Buffer> buffer; // Vector of Vulkan buffers for style data
        std::vector<uint8_t*> mapped; // Vector of mapped memory pointers for the buffers
    };

    struct PushConstants {
        uint32_t rectIndex;
        float zIndex;
    };

    constexpr size_t MAX_ELEMENTS = 100;

    constexpr vk::DeviceSize GUI_VERTEX_BUFFER_SIZE = sizeof(GUIVertex);
    constexpr vk::DeviceSize GUI_GEOMETRY_BUFFER_SIZE = sizeof(Geometry);
    constexpr vk::DeviceSize GUI_INDEX_BUFFER_SIZE = sizeof(Indices);
    constexpr vk::DeviceSize GUI_STYLE_BUFFER_SIZE = sizeof(GUIStyle);

    class GUI {
    public:
        explicit GUI(gpu::vulkan::GraphicsDevice& gpu);
        ~GUI() = default;

        void addElement(float x, float y, float width, float height, float zIndex, uint32_t styleIndex);
        void addStyle(const std::string& name, const GUIStyle& style, uint32_t inFlight);
         gpu::vulkan::Buffer* getStyleBuffer(uint32_t index, uint32_t frameIndex);

        void init();
        void update() const;
        void draw(const vk::raii::CommandBuffer& cmd) const;

    private:
        gpu::vulkan::GraphicsDevice& _gpu;
        std::vector<GUIStyleBuffer> _styleContainer{};
        std::optional<vk::raii::DescriptorSetLayout> _descriptorSetLayout{};
        std::optional<vk::raii::PipelineLayout> _pipelineLayout{};
        std::optional<vk::raii::Pipeline> _graphicsPipeline{};
        gpu::vulkan::Buffer _vertexBuffer{};
        gpu::vulkan::Buffer _indexBuffer{};
        std::optional<vk::raii::DescriptorPool> _descriptorPool{};

        gpu::vulkan::Image textureImage{};
        std::optional<vk::raii::Sampler> textureSampler{};
        std::vector<gpu::vulkan::Buffer> uniformBuffers;
        std::vector<uint8_t *> uniformBuffersMapped;
        std::vector<vk::raii::DescriptorSet> descriptorSets;

        std::vector<gpu::vulkan::Buffer> uniformBuffers2;
        std::vector<uint8_t *> uniformBuffersMapped2;
        std::vector<vk::raii::DescriptorSet> descriptorSets2;

        std::vector<GUIElement> guiElements;

        void createDescriptorSetLayout();
        void createPipelineLayout();
        void createPipeline();
        void createVertexBuffer();
        void createIndexBuffer();

        void createTextureImage();
        void createTextureImageView();
        void createTextureSampler();

        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();

        static vk::DeviceSize calculateTransformBufferSize();
    };

}

#endif //UFOX_GUI_HPP
