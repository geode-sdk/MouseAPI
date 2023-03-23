#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include "../include/API.hpp"

using namespace geode::prelude;
using namespace mouse;

struct $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;
        
        this->getChildByID("main-menu")
            ->addChild(EventListenerNode<MouseEventFilter>::create(
                [](MouseEvent* ev) {
                    log::info("{}", ev);
                    return ListenerResult::Propagate;
                }
            ));

        return true;
    }
};
