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

struct CCEGLViewModify;
struct CCTouchDispatcherModify;

namespace mouse {
    enum class MouseButton {
        Left    = 0,
        Right   = 1,
        Middle  = 2,
        Back    = 3,
        Forward = 4,
    };

    enum class MouseResult {
        Swallow = 2,
        Eat     = 1,
        Leave   = 0,
    };

    static cocos2d::ccTouchType CCTOUCHOTHER = 
        static_cast<cocos2d::ccTouchType>(85);

    class Mouse;
    class MouseEventFilter;

    class MOUSEAPI_DLL MouseAttributes : public cocos2d::CCObject {
    protected:
        cocos2d::CCNode* m_node;
    
    public:
        static MouseAttributes* from(cocos2d::CCNode* node);

        bool isHeld(MouseButton button) const;
        bool isHovered() const;

        void addHeld(MouseButton button);
        void removeHeld(MouseButton button);
        void clearHeld();
        void setHovered(bool hovered);
    };

    class MOUSEAPI_DLL MouseEvent : public geode::Event {
    protected:
        bool m_swallow = false;
        cocos2d::CCNode* m_target;
        cocos2d::CCPoint m_position;

        MouseEvent(
            cocos2d::CCNode* target,
            cocos2d::CCPoint const& position
        );

        void swallow();
        void updateTouch(cocos2d::CCTouch* touch) const;
        virtual void dispatchTouch(cocos2d::CCNode* target, cocos2d::CCTouch* touch) const = 0;

        geode::EventListenerPool* getPool() const override;

        friend class MouseEventFilter;
        friend struct CCTouchDispatcherModify;
    
    public:
        bool isSwallowed() const;
        cocos2d::CCPoint getPosition() const;
        cocos2d::CCNode* getTarget() const;

        cocos2d::CCTouch* createTouch() const;
        cocos2d::CCEvent* createEvent() const;

        using Filter = MouseEventFilter;
    };

    class MOUSEAPI_DLL MouseClickEvent : public MouseEvent {
    protected:
        MouseButton m_button;
        bool m_down;

        void dispatchTouch(cocos2d::CCNode* target, cocos2d::CCTouch* touch) const override;
    
    public:
        MouseClickEvent(
            MouseButton button,
            bool down,
            cocos2d::CCPoint const& position
        );
        MouseClickEvent(
            cocos2d::CCNode* target, MouseButton button,
            bool down,
            cocos2d::CCPoint const& position
        );

        MouseButton getButton() const;
        bool isDown() const;
    };

    class MOUSEAPI_DLL MouseMoveEvent : public MouseEvent {
    protected:
        void dispatchTouch(cocos2d::CCNode* target, cocos2d::CCTouch* touch) const override;

    public:
        MouseMoveEvent(
            cocos2d::CCPoint const& position
        );
        MouseMoveEvent(
            cocos2d::CCNode* target,
            cocos2d::CCPoint const& position
        );
    };
    
    class MOUSEAPI_DLL MouseScrollEvent : public MouseEvent {
    protected:
        float m_deltaY;
        float m_deltaX;

        void dispatchTouch(cocos2d::CCNode* target, cocos2d::CCTouch* touch) const override;
    
    public:
        MouseScrollEvent(
            float deltaY, float deltaX,
            cocos2d::CCPoint const& position
        );
        MouseScrollEvent(
            cocos2d::CCNode* target,
            float deltaY, float deltaX,
            cocos2d::CCPoint const& position
        );

        float getDeltaY() const;
        float getDeltaX() const;
    };

    class MOUSEAPI_DLL MouseHoverEvent : public MouseEvent {
    protected:
        bool m_enter;
        
        void dispatchTouch(cocos2d::CCNode* target, cocos2d::CCTouch* touch) const override;
    
    public:
        MouseHoverEvent(
            cocos2d::CCNode* target, bool enter,
            cocos2d::CCPoint const& position
        );

        bool isEnter() const;
        bool isLeave() const;
    };

    class MOUSEAPI_DLL MouseEventFilter : public geode::EventFilter<MouseEvent> {
    protected:
        std::optional<geode::WeakRef<cocos2d::CCNode>> m_target;
        geode::Ref<cocos2d::CCTouch> m_eaten = nullptr;
        bool m_ignorePosition = false;
        size_t m_filterIndex = 0;

    public:
        using Callback = MouseResult(MouseEvent*);

        geode::ListenerResult handle(geode::utils::MiniFunction<Callback> fn, MouseEvent* event);
        geode::EventListenerPool* getPool() const;
        MouseEventFilter(cocos2d::CCNode* target, bool ignorePosition = false);
        ~MouseEventFilter();

        std::optional<geode::WeakRef<cocos2d::CCNode>> getTarget() const;
        std::vector<int> getTargetPriority() const;
        size_t getFilterIndex() const;
    };

    class MOUSEAPI_DLL Mouse {
    protected:
        std::unordered_set<MouseButton> m_heldButtons;
        static inline std::atomic_bool s_updating = false;

        friend struct ::CCEGLViewModify;

    public:
        static Mouse* get();

        geode::EventListener<MouseEventFilter>* getCapturing() const;
        cocos2d::CCNode* getCapturingNode() const;
        void capture(geode::EventListener<MouseEventFilter>* listener);
        void release(geode::EventListener<MouseEventFilter>* listener);

        bool isHeld(MouseButton button) const;

        static void updateListeners();
    };
}

template <>
struct json::Serialize<mouse::MouseButton> {
    static json::Value MOUSEAPI_DLL to_json(mouse::MouseButton const& button);
    static mouse::MouseButton MOUSEAPI_DLL from_json(json::Value const& button);
};
