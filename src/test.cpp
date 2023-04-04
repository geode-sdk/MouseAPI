#include <Geode/modify/MenuLayer.hpp>
#include "../include/API.hpp"

using namespace geode::prelude;
using namespace mouse;

#ifndef MOUSEAPI_TEST

class HoveredNode : public CCNode {
protected:
    CCLabelBMFont* m_label;

    bool init() {
        if (!CCNode::init())
            return false;
        
        this->setContentSize({ 100.f, 50.f });

        m_label = CCLabelBMFont::create("Hi", "bigFont.fnt");
        m_label->setPosition(m_obContentSize / 2);
        this->addChild(m_label);

        this->template addEventListener<MouseEventFilter>([=](MouseEvent* event) {
            if (auto hover = typeinfo_cast<MouseHoverEvent*>(event)) {
                if (hover->isEnter()) {
                    m_label->setString("Hovered!");
                }
                else {
                    m_label->setString("Hi");
                }
            }
            if (MouseAttributes::from(this)->isHeld(MouseButton::Right)) {
                m_label->setString("Right-clicked!");
            }
            return MouseResult::Swallow;
        });

        return true;
    }

public:
    static HoveredNode* create() {
        auto ret = new HoveredNode();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

struct $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;
        
        this->getChildByID("main-title")
            ->setAttribute("hjfod.mouse-api/tooltip", "Omg tooltips");

        this->getChildByID("main-menu")
            ->setAttribute("hjfod.mouse-api/context-menu",
                json::Array {
                    json::Object {
                        { "text", "Click to Work?" },
                        { "frame", "GJ_infoIcon_001.png" },
                        { "click", "my-event-id"_spr },
                    },
                    json::Object {
                        { "text", "Click to Work?" },
                        { "frame", "GJ_infoIcon_001.png" },
                        { "sub-menu", json::Array {
                            json::Object {
                                { "text", "Sub menu!!" },
                            }
                        } },
                    },
                    json::Object {
                        { "text", "Quit Game" },
                        { "click", "quit-game"_spr },
                    },
                }
            );
        
        auto node = HoveredNode::create();
        node->setPosition(60, 80);
        node->setZOrder(15);
        this->addChild(node);

        return true;
    }
};

#endif
