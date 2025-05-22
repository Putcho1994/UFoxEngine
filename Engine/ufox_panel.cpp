//
// Created by b-boy on 22.05.2025.
//

#include <Engine/ufox_panel.hpp>

namespace ufox {
    Panel::Panel() = default;
    Panel::~Panel() = default;

    void Panel::onResize(const glm::vec2& newSize) {

        if (children.empty()) {
            transform.width = newSize.x - transform.x;
            transform.height = newSize.y - transform.y;
        }
        else {
            for (const auto& child: children) {
                const auto& [colum, row] = child.layout;

            }
        }

        rootElement.transform = transform;
    }

    void Panel::onUpdate(const SDL_Event& event) {

    }

    void Panel::onRender() {

    }

}

