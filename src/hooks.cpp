#include <Geode/modify/CCLayer.hpp>
#include <Geode/modify/CCMenu.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>
#include <Geode/modify/AchievementNotifier.hpp>
#include <Geode/utils/cocos.hpp>
#include "../include/API.hpp"

using namespace geode::prelude;
using namespace mouse;

struct $modify(CCTouchDispatcher) {
    void addStandardDelegate(CCTouchDelegate* delegate, int prio) {
        CCTouchDispatcher::addStandardDelegate(delegate, prio);
        // CCMenu is handled specially
        if (!typeinfo_cast<CCMenu*>(delegate)) {
            if (auto node = typeinfo_cast<CCNode*>(delegate)) {
                node->template addEventListener<MouseEventFilter>([=](MouseEvent* event) {
                    return ListenerResult::Propagate;
                });
                Mouse::updateListeners();
            }
        }
    }

    void addTargetedDelegate(CCTouchDelegate* delegate, int prio, bool swallows) {
        CCTouchDispatcher::addTargetedDelegate(delegate, prio, swallows);
        // CCMenu is handled specially
        if (typeinfo_cast<CCMenu*>(delegate)) return;
        if (auto node = typeinfo_cast<CCNode*>(delegate)) {
            if (swallows) {
                node->template addEventListener<MouseEventFilter>([=](MouseEvent*) {
                    return ListenerResult::Stop;
                });
            }
            else {
                node->template addEventListener<MouseEventFilter>([=](MouseEvent* event) {
                    return ListenerResult::Propagate;
                });
            }
            Mouse::updateListeners();
        }
    }

    // void removeDelegate(CCTouchDelegate* delegate) {
    //     if (auto node = typeinfo_cast<CCNode*>(delegate)) {
    //         if (auto listener = node->template getAttribute<EventListenerProtocol*>(
    //             "hjfod.mouse-api/listener"
    //         )) {
    //             node->removeEventListener(listener.value());
    //             Mouse::release(node);
    //         }
    //     }
    //     CCTouchDispatcher::removeDelegate(delegate);
    // }
};

struct $modify(CCMenu) {
    bool initWithArray(CCArray* items) {
        if (!CCMenu::initWithArray(items))
            return false;

        this->template addEventListener<MouseEventFilter>([=](MouseEvent* ev) {
            CCTouch touch;
            touch.m_point = CCDirector::get()->convertToUI(ev->getPosition());
            if (auto item = this->itemForTouch(&touch)) {
                return ListenerResult::Stop;
            }
            return ListenerResult::Propagate;
        }, true);
        Mouse::updateListeners();
        
        return true;
    }
};

struct $modify(AchievementNotifier) {
    void willSwitchToScene(CCScene* scene) {
        AchievementNotifier::willSwitchToScene(scene);
        Mouse::updateListeners();
    }
};
