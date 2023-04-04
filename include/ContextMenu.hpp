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
        bool m_hovered = false;
        cocos2d::CCNode* m_icon = nullptr;
        cocos2d::CCLabelBMFont* m_label = nullptr;

        bool init();

        void draw() override;

    public:
        void setIcon(cocos2d::CCNode* icon);
        void setText(std::string const& text);

        float getPreferredWidth();
        void fitToWidth(float width);

        virtual void hover();
        virtual void unhover();
        virtual void select() = 0;
    };

    using ItemRef = geode::Ref<ContextMenuItem>;

    class MOUSEAPI_DLL SubMenuItem : public ContextMenuItem {
    protected:
        std::vector<ItemRef> m_items;
        geode::Ref<ContextMenu> m_menu;

        bool init(std::vector<ItemRef> const& items);
    
    public:
        static SubMenuItem* create(std::vector<ItemRef> const& items);

        void select() override;
        void hover() override;
        void unhover() override;
    };

    class MOUSEAPI_DLL ContextMenu : public cocos2d::CCNode {
    protected:
        std::vector<ItemRef> m_items;
        cocos2d::CCNode* m_container;
        static inline geode::Ref<ContextMenu> s_current = nullptr;

        bool init(std::vector<ItemRef> const& items);

    public:
        static ContextMenu* create(std::vector<ItemRef> const& items);
        static ContextMenu* get(bool create = true);

        void show(cocos2d::CCPoint const& pos);
        void hide();
    };
}

