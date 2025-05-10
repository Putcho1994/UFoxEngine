//
// Created by b-boy on 04.05.2025.
//
#pragma once
#include <glm/glm.hpp>
#include <Engine/ufox_graphic.hpp>

namespace ufox::renderer::gui {
    struct Vertex {
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 texCoord;

        static vk::VertexInputBindingDescription getBindingDescription() {
            vk::VertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = vk::VertexInputRate::eVertex;
            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
            attributeDescriptions[0].offset = offsetof(Vertex, position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32B32A32Sfloat;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
            return attributeDescriptions;
        }
    };

    struct Size {
        int width;
        int height;
        int min_width;
        int min_height;
        int max_width;
        int max_height;
    };

    struct Rect {
        glm::vec2 position;
        glm::vec2 size;
    };

    struct Margin {
        int top;
        int right;
        int bottom;
        int left;
    };

    struct Padding {
        int top;
        int right;
        int bottom;
        int left;
    };

    struct Blackground {
        glm::vec4 color;

    };

    class GUIRenderer {

        GUIRenderer();
        ~GUIRenderer();

    private:

        std::optional<vk::raii::DescriptorSetLayout> descriptorSetLayout{};
        std::optional<vk::raii::PipelineLayout> pipelineLayout{};
        std::optional<vk::raii::Pipeline> graphicsPipeline{};
        std::optional<vk::raii::DescriptorPool> descriptorPool{};
        std::vector<vk::raii::DescriptorSet> descriptorSets;

        void init();


    };
}
