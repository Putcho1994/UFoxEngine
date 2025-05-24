//
// Created by b-boy on 22.05.2025.
//

#include <Engine/ufox_panel.hpp>



namespace ufox {
    Panel::Panel(InputSystem& input) : _input(input){}
    Panel::~Panel() = default;

    void Panel::onResize(const glm::vec2& newSize) {
        // Width: Check if panel is outside window width
        if (newSize.x < transform.x) {
            transform.width = 0;
            if (left) {
                transform.x = newSize.x;
                left->onResize(newSize);
            }
        } else {
            if (right) {
                transform.width = right->transform.x - transform.x;
            } else {
                transform.width = newSize.x - transform.x;
            }
        }

        // Height: Check if panel is outside window height
        if (newSize.y < transform.y) {
            transform.height = 0;
            if (top) {
                transform.y = newSize.y;
                top->onResize(newSize);
            }
        } else {
            if (bottom) {
                transform.height = bottom->transform.y - transform.y;
            } else {
                transform.height = newSize.y - transform.y;
            }
        }

        rootElement.transform = transform;
    }

    void Panel::onUpdate(const SDL_Event& event) const {
        if (right) {
            auto mousePos = _input.getMousePosition();
            float splitterStart = right->transform.x - splitterHandleOffset;
            float splitterEnd = splitterStart + splitterHandleSize;

            // Check if mouse x is within splitter handle (y not constrained for simplicity)
            if (mousePos.x >= splitterStart && mousePos.x <= splitterEnd) {
                _input.setCursor(CursorType::eEWResize);
            } else { _input.setCursor(CursorType::eDefault); }
        }
    }

    void Panel::onRender() {}

    void Panel::BridgePanel(Panel& target, AttachmentPosition position) {
        switch (position) {
            case AttachmentPosition::eTop:
                top = &target;
                target.bottom = this;
                break;
            case AttachmentPosition::eBottom:
                bottom = &target;
                target.top = this;
                break;
            case AttachmentPosition::eRight:
                right = &target;
                target.left = this;
                break;
            case AttachmentPosition::eLeft:
                left = &target;
                target.right = this;
                break;
        }
    }

}

