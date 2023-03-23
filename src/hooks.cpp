#include <Geode/modify/CCLayer.hpp>
#include <Geode/utils/cocos.hpp>
#include "../include/API.hpp"

using namespace geode::prelude;
using namespace mouse;

struct $modify(CCLayer) {
    bool init() {
        if (!CCLayer::init())
            return false;

        this->addEventListener<MouseEventFilter>([](MouseEvent*) {
            return ListenerResult::Stop;
        });
        MouseEventFilter::reorderTargets();
        
        return true;
    }
};
