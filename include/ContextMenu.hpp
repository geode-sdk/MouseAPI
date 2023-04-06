#pragma once

#include "API.hpp"
#include <Geode/DefaultInclude.hpp>
#include <Geode/loader/Dispatch.hpp>
#include <Geode/utils/cocos.hpp>

namespace mouse {
    class ContextMenu;

    using ContextMenuEvent = geode::DispatchEvent<cocos2d::CCNode*>;
    using ContextMenuFilter = geode::DispatchFilter<cocos2d::CCNode*>;

    class MOUSEAPI_DLL ContextMenuItem : public cocos2d::CCNode {
    protected:
        ContextMenu* m_parentMenu;
        cocos2d::CCNode* m_icon = nullptr;
        cocos2d::CCLabelBMFont* m_label = nullptr;

        bool init(ContextMenu* menu);

        void draw() override;

    public:
        void setIcon(cocos2d::CCNode* icon);
        void setText(std::string const& text);

        virtual float getPreferredWidth();
        virtual void fitToWidth(float width);

        virtual bool isHovered();
        virtual void hide();
        virtual void select() = 0;
    };

    using ItemRef = geode::Ref<ContextMenuItem>;

    class MOUSEAPI_DLL ActionMenuItem : public ContextMenuItem {
    protected:
        std::string m_eventID;

        bool init(ContextMenu* menu, std::string const& eventID);
    
    public:
        static ActionMenuItem* create(ContextMenu* menu, std::string const& eventID);

        void select() override;
    };

    class MOUSEAPI_DLL SubMenuItem : public ContextMenuItem {
    protected:
        std::vector<ItemRef> m_items;
        ContextMenu* m_menu = nullptr;
        cocos2d::CCSprite* m_arrow;

        bool init(ContextMenu* menu, std::vector<ItemRef> const& items);
    
        void draw() override;

    public:
        static SubMenuItem* create(ContextMenu* menu, std::vector<ItemRef> const& items);

        float getPreferredWidth() override;
        void fitToWidth(float width) override;

        bool isHovered() override;
        void select() override;
        void hide() override;
    };

    class MOUSEAPI_DLL ContextMenu : public cocos2d::CCNode {
    protected:
        geode::Ref<cocos2d::CCNode> m_target;
        std::vector<ItemRef> m_items;
        cocos2d::CCNode* m_container;
        cocos2d::extension::CCScale9Sprite* m_bg;

        bool init(cocos2d::CCNode* target);

        ActionMenuItem* createError(std::string const& msg);
        void loadItems(std::vector<ItemRef> const& items);
        std::vector<ItemRef> parseItems(json::Value const& value);

    public:
        static ContextMenu* create(cocos2d::CCNode* target);
        static ContextMenu* create(cocos2d::CCNode* target, std::vector<ItemRef> const& items);
        static ContextMenu* create(cocos2d::CCNode* target, json::Value const& items);

        cocos2d::CCNode* getTarget() const;

        bool isHovered();
        void show(cocos2d::CCPoint const& pos);
        void hide();
    };
}

