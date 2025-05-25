//
// Created by b-boy on 17.05.2025.
//
#pragma once

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <SDL3/SDL_filesystem.h>

#include <vector>

namespace ufox::core {

    struct TransformRect {
        float x;
        float y;
        float width;
        float height;

        [[nodiscard]] glm::mat4 getMatrix() const {
            return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f)) *
                   glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
        }
    };

    enum class DraggerDirection {
        eBoth,
        eVertical,
        eHorizontal,
    };

    struct DraggerHandle {

        DraggerHandle() = default;
        DraggerHandle(DraggerDirection direction) : direction(direction) {}

        TransformRect transform{0,0,0,0};
        DraggerDirection direction{DraggerDirection::eBoth};

        [[nodiscard]] bool isHovering(const glm::vec2& point) const {
            return point.x >= transform.x && point.x <= transform.x + transform.width &&
                   point.y >= transform.y && point.y <= transform.y + transform.height;
        }
    };

    struct TransformMatrix {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    constexpr vk::DeviceSize RECT_BUFFER_SIZE = sizeof(TransformRect);
    constexpr vk::DeviceSize TRANSFORM_BUFFER_SIZE = sizeof(TransformMatrix);

    static std::vector<char> loadShader(const std::string& filename) {
        std::string path = SDL_GetBasePath() + filename;
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open shader: " + path);

        size_t size = file.tellg();
        if (size > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
            throw std::runtime_error("File size exceeds maximum streamsize limit");
        }

        std::vector<char> buffer(size);
        file.seekg(0);
        file.read(buffer.data(), static_cast<std::streamsize>(size));
        file.close();
        return buffer;
    }
}
