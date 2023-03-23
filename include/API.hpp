#pragma once

#include <Geode/loader/Event.hpp>

#ifdef GEODE_IS_WINDOWS
    #ifdef HJFOD_MOUSEAPI_EXPORTING
        #define MOUSEAPI_DLL __declspec(dllexport)
    #else
        #define MOUSEAPI_DLL __declspec(dllimport)
    #endif
#else
    #define MOUSEAPI_DLL
#endif

namespace mouse {
    enum class MouseButton {
        Left    = 0,
        Right   = 1,
        Middle  = 2,
        Back    = 3,
        Forward = 4,
    };

    class MOUSEAPI_DLL MouseEvent : public geode::Event {
    protected:
        bool m_swallow = false;
        cocos2d::CCPoint m_position;

        MouseEvent(cocos2d::CCPoint const& position);
    
    public:
        void swallow();
        bool isSwallowed() const;
        cocos2d::CCPoint getPosition() const;
    };

    class MOUSEAPI_DLL MouseClickEvent : public MouseEvent {
    protected:
        MouseButton m_button;
        bool m_down;
    
    public:
        MouseClickEvent(MouseButton button, bool down, cocos2d::CCPoint const& position);

        MouseButton getButton() const;
        bool isDown() const;
    };

    class MOUSEAPI_DLL MouseMoveEvent : public MouseEvent {
    public:
        MouseMoveEvent(cocos2d::CCPoint const& position);
    };

    class MOUSEAPI_DLL MouseHoverEvent : public MouseEvent {
    protected:
        bool m_enter;
    
    public:
        MouseHoverEvent(bool enter, cocos2d::CCPoint const& position);

        bool isEnter() const;
        bool isLeave() const;
    };
    
    class MOUSEAPI_DLL MouseScrollEvent : public MouseEvent {
    protected:
        float m_deltaY;
        float m_deltaX;
    
    public:
        MouseScrollEvent(float deltaY, float deltaX, cocos2d::CCPoint const& position);

        float getDeltaY() const;
        float getDeltaX() const;
    };

    class MOUSEAPI_DLL MouseEventFilter : public geode::EventFilter<MouseEvent> {
    protected:
        cocos2d::CCNode* m_target;
        bool m_hovered = false;

    public:
        using Callback = geode::ListenerResult(MouseEvent*);

        geode::ListenerResult handle(geode::utils::MiniFunction<Callback> fn, MouseEvent* event);
        MouseEventFilter(cocos2d::CCNode* target);
    };

    class MOUSEAPI_DLL Mouse {
    protected:
        geode::Ref<cocos2d::CCNode> m_swallowing;
    
    public:
        static Mouse* get();

        cocos2d::CCNode* getCapturing() const;

        static void capture(cocos2d::CCNode* target);
        static void release(cocos2d::CCNode* target);
    };
}
