#include <Geode/modify/CCLayer.hpp>
#include <Geode/modify/CCMenu.hpp>
#include <Geode/modify/CCMenuItemSprite.hpp>
#include <Geode/utils/cocos.hpp>
#include "../include/API.hpp"

using namespace geode::prelude;
using namespace mouse;

struct $modify(CCLayer) {
    bool init() {
        if (!CCLayer::init())
            return false;

        if (!typeinfo_cast<CCMenu*>(this)) {
            this->template addEventListener<MouseEventFilter>([&](MouseEvent*) {
                return ListenerResult::Stop;
            });
            Mouse::reorderTargets();
        }
        
        return true;
    }
};

struct $modify(CCMenuItemSprite) {
    bool initWithNormalSprite(CCNode* a, CCNode* b, CCNode* c, CCObject* sender, SEL_MenuHandler selector) {
        if (!CCMenuItemSprite::initWithNormalSprite(a, b, c, sender, selector))
            return false;

        this->template addEventListener<MouseEventFilter>([&](MouseEvent* ev) {
            return ListenerResult::Stop;
        });
        Mouse::reorderTargets();
        
        return true;
    }
};

// struct $modify(CCMenu) {
//     bool initWithArray(CCArray* items) {
//         if (!CCMenu::initWithArray(items))
//             return false;

//         this->template addEventListener<MouseEventFilter>([&](MouseEvent* ev) {
//             CCTouch touch;
//             touch.m_point = CCDirector::get()->convertToUI(ev->getPosition());
//             if (auto item = this->itemForTouch(&touch)) {
//                 log::info(__FUNCTION__);
//                 return ListenerResult::Stop;
//             }
//             return ListenerResult::Propagate;
//         });
//         Mouse::reorderTargets();
        
//         return true;
//     }
// };
