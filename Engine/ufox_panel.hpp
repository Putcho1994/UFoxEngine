//
// Created by b-boy on 22.05.2025.
//

#ifndef UFOX_PANEL_HPP
#define UFOX_PANEL_HPP
#pragma once
#include <vector>
#include <Engine/ufox_engine_core.hpp>
#include <Engine/ufox_gui.hpp>


namespace ufox {
    class Panel {
    public:
        Panel();
        ~Panel();

        void setTransform(const core::TransformRect& newTransform) {
            transform = newTransform;
            rootElement.transform = transform; // Update root element's transform
        }

        void onResize(const glm::vec2& newSize);
        void onUpdate(const SDL_Event& event);
        void onRender();

        core::TransformRect transform{};
        gui::GUIElement rootElement{};
        bool dockable{false};
        Panel* parent{nullptr};
        std::vector<Panel> children{};
        std::pair<uint32_t, uint32_t> layout{};
    };
}


#endif //UFOX_PANEL_HPP
