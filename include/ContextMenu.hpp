#pragma once

#include "API.hpp"
#include <Geode/DefaultInclude.hpp>
#include <Geode/loader/Dispatch.hpp>
#include <Geode/utils/cocos.hpp>

namespace mouse {
    class ContextMenu;

    using ContextMenuEvent = geode::DispatchEvent<cocos2d::CCNode*>;
    using ContextMenuFilter = geode::DispatchFilter<cocos2d::CCNode*>;

    struct ContextMenuStyle {
        float padding = 3.f;
        float height = 15.f;
        float minWidth = 50.f;
        float maxWidth = 150.f;
        float maxHeight = 140.f;
        float itemGap = 0.f;
        std::string fontName = "chatFont.fnt";
        std::string arrowSprite = "arrow.png"_spr;
        bool flipArrow = false;
        float arrowSize = 7.5f;
        std::string bgSprite = "square02b_small.png";
        cocos2d::ccColor4B bgColor = { 0, 0, 0, 185 };
        cocos2d::ccColor4B textColor = { 255, 255, 255, 255 };
        cocos2d::ccColor4B hoverColor = { 255, 255, 255, 45 };
        cocos2d::ccColor4B arrowColor = { 255, 255, 255, 115 };
    };

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
        ContextMenuStyle m_style;

        bool init(cocos2d::CCNode* target, ContextMenuStyle const& style);

        ActionMenuItem* createError(std::string const& msg);
        void loadItems(std::vector<ItemRef> const& items);
        std::vector<ItemRef> parseItems(json::Value const& value);

    public:
        static ContextMenu* create(
            cocos2d::CCNode* target,
            ContextMenuStyle const& style = ContextMenuStyle()
        );
        static ContextMenu* create(
            cocos2d::CCNode* target,
            std::vector<ItemRef> const& items,
            ContextMenuStyle const& style = ContextMenuStyle()
        );
        static ContextMenu* create(
            cocos2d::CCNode* target,
            json::Value const& items,
            ContextMenuStyle const& style = ContextMenuStyle()
        );

        cocos2d::CCNode* getTarget() const;
        ContextMenuStyle const& getStyle() const;

        bool isHovered();
        void show(cocos2d::CCPoint const& pos);
        void hide();
    };
}

