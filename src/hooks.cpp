#include <Geode/modify/CCLayer.hpp>
#include <Geode/modify/CCMenu.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>
#include <Geode/modify/AchievementNotifier.hpp>
#include <Geode/modify/CCScrollLayerExt.hpp>
#include <Geode/utils/cocos.hpp>
#include "../include/API.hpp"

using namespace geode::prelude;
using namespace mouse;

struct $modify(CCTouchDispatcher) {
    void addStandardDelegate(CCTouchDelegate* delegate, int prio) {
        CCTouchDispatcher::addStandardDelegate(delegate, prio);
        if (auto node = typeinfo_cast<CCNode*>(delegate)) {
            if (node->getEventListener("mouse"_spr)) return;
            node->template addEventListener<MouseEventFilter>(
                "mouse"_spr,
                [=](MouseEvent*) {
                    return MouseResult::Eat;
                }
            );
            Mouse::updateListeners();
        }
    }

    void addTargetedDelegate(CCTouchDelegate* delegate, int prio, bool swallows) {
        CCTouchDispatcher::addTargetedDelegate(delegate, prio, swallows);
        // handle CCScrollLayerExt specially since it's quite wacky
        if (auto scroll = typeinfo_cast<CCScrollLayerExt*>(delegate)) {
            scroll->template addEventListener<MouseEventFilter>(
                "mouse"_spr,
                [=](MouseEvent* event) {
                    if (!scroll->m_disableMovement) {
                        if (scroll->boundingBox().containsPoint(
                            scroll->convertToNodeSpace(event->getPosition())
                        )) {
                            return MouseResult::Eat;
                        }
                    }
                    return MouseResult::Leave;
                },
                true
            );
        }
        else if (auto node = typeinfo_cast<CCNode*>(delegate)) {
            if (node->getEventListener("mouse"_spr)) return;
            if (swallows) {
                node->template addEventListener<MouseEventFilter>(
                    "mouse"_spr,
                    [=](MouseEvent*) {
                        return MouseResult::Swallow;
                    }
                );
            }
            else {
                node->template addEventListener<MouseEventFilter>(
                    "mouse"_spr,
                    [=](MouseEvent*) {
                        return MouseResult::Eat;
                    }
                );
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

struct $modify(CCScrollLayerExt) {};

struct $modify(CCMenu) {
    bool initWithArray(CCArray* items) {
        if (!CCMenu::initWithArray(items))
            return false;

        this->template addEventListener<MouseEventFilter>(
            "mouse"_spr,
            [=](MouseEvent* ev) {
                CCTouch touch;
                touch.m_point = CCDirector::get()->convertToUI(ev->getPosition());
                if (auto item = this->itemForTouch(&touch)) {
                    return MouseResult::Swallow;
                }
                return MouseResult::Leave;
            },
            true
        );
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
