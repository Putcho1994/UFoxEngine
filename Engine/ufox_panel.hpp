//
// Created by b-boy on 22.05.2025.
//

#ifndef UFOX_PANEL_HPP
#define UFOX_PANEL_HPP
#pragma once
#include <vector>
#include <Engine/ufox_engine_core.hpp>
#include <Engine/ufox_gui.hpp>
#include <Engine/ufox_inputSystem.hpp>
#include <SDL3/SDL_mouse.h>


namespace ufox {
    enum class AttachmentPosition {
        eTop,
        eBottom,
        eRight,
        eLeft
    };

    class Panel {
    public:
        Panel(InputSystem& input);
        ~Panel();

        void setTransform(const core::TransformRect& newTransform) {
            transform = newTransform;
            rootElement.transform = transform; // Update root element's transform
        }

        void onResize(const glm::vec2& newSize);
        void onUpdate(const SDL_Event& event) const;
        void onRender();
        void BridgePanel(Panel& target, AttachmentPosition position);

        core::TransformRect transform{};
        gui::GUIElement rootElement{};
        bool dockable{false};
        Panel* top{nullptr};
        Panel* bottom{nullptr};
        Panel* right{nullptr};
        Panel* left{nullptr};

    private:
        InputSystem& _input;
        static constexpr float splitterHandleOffset = 5.0f; // Pixels left of right neighbor's x
        static constexpr float splitterHandleSize = 10.0f; // Width of splitter handle
    };


}
#endif //UFOX_PANEL_HPP
