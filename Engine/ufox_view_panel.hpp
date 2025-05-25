//
// Created by b-boy on 22.05.2025.
//

#ifndef UFOX_PANEL_HPP
#define UFOX_PANEL_HPP
#pragma once
#include <vector>
#include <Engine/ufox_engine_core.hpp>
#include <Engine/ufox_gui_renderer.hpp>
#include <Engine/ufox_input_system.hpp>
#include <SDL3/SDL_mouse.h>


namespace ufox {
    enum class AttachmentPosition {
        eTop,
        eBottom,
        eRight,
        eLeft
    };

    enum class Align {
        eColumn,
        eRow,
        eColumnReverse,
        eRowReverse
    };

    class ViewPanel {
    public:
        ViewPanel(InputSystem& input);
        ~ViewPanel();

        void setTransform(core::TransformRect newTransform) {
            transform = newTransform;
            rootElement.transform = transform; // Update root element's transform
        }

        void onResize(const glm::vec2& newSize);
        void onUpdate();
        void onRender();
        void BridgePanel(ViewPanel& target, AttachmentPosition position);

        core::TransformRect transform{};
        gui::GUIElement rootElement{};
        bool dockable{false};

        Align splitterDirection{Align::eColumn};

        ViewPanel* parent{nullptr};
        std::vector<ViewPanel*> hierarchy{};


        ViewPanel* top{nullptr};
        ViewPanel* bottom{nullptr};
        ViewPanel* right{nullptr};
        ViewPanel* left{nullptr};

    private:
        InputSystem& _input;
        static constexpr float splitterHandleOffset = 5.0f; // Pixels left of right neighbor's x
        static constexpr float splitterHandleSize = 10.0f; // Width of splitter handle

        glm::vec2 currentWindowSize{0};
        bool enabledSplitterResizing{false};
    };


}
#endif //UFOX_PANEL_HPP
