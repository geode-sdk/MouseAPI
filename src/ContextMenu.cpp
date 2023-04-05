#include "../include/ContextMenu.hpp"

using namespace geode::prelude;
using namespace mouse;

bool ContextMenuItem::init(ContextMenu* menu) {
    if (!CCNode::init())
        return false;

    m_parentMenu = menu;
    
    this->addChild(SpacerNode::create());
    this->setLayout(RowLayout::create());

    return true;
}

float ContextMenuItem::getPreferredWidth() {
    return this->getLayout()->getSizeHint(this).width;
}

void ContextMenuItem::fitToWidth(float width) {
    this->setContentSize({ width, 20.f });
}

void ContextMenuItem::setIcon(cocos2d::CCNode* icon) {
    if (m_icon) {
        m_icon->removeFromParent();
    }
    m_icon = icon;
    if (icon) {
        this->insertBefore(icon, nullptr);
        limitNodeSize(icon, { 17.5f, 17.5f }, 1.f, .1f);
    }
    this->updateLayout();
}

void ContextMenuItem::setText(std::string const& text) {
    if (!m_label) {
        m_label = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        if (m_icon) {
            this->insertAfter(m_label, m_icon);
        }
        else {
            this->insertBefore(m_label, nullptr);
        }
        this->updateLayout();
    }
}

void ContextMenuItem::draw() {
    if (m_hovered) {
        ccDrawSolidRect({ 0, 0 }, m_obContentSize, { 1.f, 1.f, 1.f, .25f });
    }
    CCNode::draw();
}

void ContextMenuItem::hover() {
    m_hovered = true;
}

void ContextMenuItem::unhover() {
    m_hovered = false;
}

void ContextMenuItem::hide() {}

bool ActionMenuItem::init(ContextMenu* menu, std::string const& eventID) {
    if (!ContextMenuItem::init(menu))
        return false;

    m_eventID = eventID;

    return true;
}

ActionMenuItem* ActionMenuItem::create(ContextMenu* menu, std::string const& eventID) {
    auto ret = new ActionMenuItem();
    if (ret && ret->init(menu, eventID)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void ActionMenuItem::select() {
    ContextMenuEvent(m_eventID, m_parentMenu->getTarget()).post();
    m_parentMenu->hide();
}

bool SubMenuItem::init(ContextMenu* menu, std::vector<ItemRef> const& items) {
    if (!ContextMenuItem::init(menu))
        return false;

    m_items = items;

    return true;
}

SubMenuItem* SubMenuItem::create(ContextMenu* menu, std::vector<ItemRef> const& items) {
    auto ret = new SubMenuItem;
    if (ret && ret->init(menu, items)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void SubMenuItem::select() {
    if (!m_menu) {
        auto bbox = this->boundingBox();
        m_menu = ContextMenu::create(m_parentMenu->getTarget(), m_items);
        m_menu->show({ bbox.getMaxX(), bbox.getMaxY() });
    }
}

void SubMenuItem::hover() {
    ContextMenuItem::hover();
    this->select();
}

void SubMenuItem::unhover() {
    ContextMenuItem::unhover();
    m_menu->hide();
    m_menu = nullptr;
}

void SubMenuItem::hide() {
    m_menu->hide();
    m_menu = nullptr;
}

bool ContextMenu::init(CCNode* target) {
    if (!CCNode::init())
        return false;
    
    m_target = target;

    m_bg = CCScale9Sprite::create("square02b_small-uhd.png", { 0, 0, 40, 40 });
    m_bg->setColor({ 0, 0, 0 });
    m_bg->setOpacity(155);
    m_bg->setScale(.4f);
    this->addChild(m_bg);

    m_container = CCNode::create();
    m_container->setLayout(
        RowLayout::create()
            ->setGrowCrossAxis(true)
            ->setAxisAlignment(AxisAlignment::Even)
            ->setGap(0.f)
    );
    this->addChild(m_container);

    this->setAnchorPoint({ 0.f, 1.f });

    return true;
}

ContextMenu* ContextMenu::create(cocos2d::CCNode* target) {
    auto ret = new ContextMenu;
    if (ret && ret->init(target)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

ContextMenu* ContextMenu::create(cocos2d::CCNode* target, json::Value const& items) {
    auto ret = ContextMenu::create(target);
    ret->loadItems(ret->parseItems(items));
    return ret;
}

ContextMenu* ContextMenu::create(CCNode* target, std::vector<ItemRef> const& items) {
    auto ret = ContextMenu::create(target);
    ret->loadItems(items);
    return ret;
}

void ContextMenu::loadItems(std::vector<ItemRef> const& items) {
    m_items = items;

    float width = 0.f;
    for (auto& item : items) {
        auto w = item->getPreferredWidth();
        if (w > width) {
            width = w;
        }
    }
    width = clamp(width, 100.f, 160.f);

    for (auto& item : items) {
        m_container->addChild(item);
        item->fitToWidth(width);
        item->updateLayout();
    }
    auto containerHeight = static_cast<RowLayout*>(m_container->getLayout())
        ->getSizeHint(m_container).height;

    m_container->setContentSize({ width, containerHeight });
    m_container->updateLayout();

    auto height = clamp(containerHeight, 0.f, 200.f);

    this->setContentSize({ width, height });
    m_bg->setContentSize(m_obContentSize / m_bg->getScale());
    m_bg->setPosition(m_obContentSize / 2);
}

ActionMenuItem* ContextMenu::createError(std::string const& msg) {
    auto item = ActionMenuItem::create(this, "");
    item->setText(msg);
    return item;
}

std::vector<ItemRef> ContextMenu::parseItems(json::Value const& value) {
    std::vector<ItemRef> items;

    if (!value.is_array()) {
        return { this->createError("Context menu is not an array") };
    }

    for (auto item : value.as_array()) {
        if (!item.is_object()) {
            items.push_back(this->createError("Item is not an object"));
            continue;
        }
        auto obj = item.as_object();
        bool hasTextOrIcon = false;
        ContextMenuItem* item;
        if (obj.count("sub-menu")) {
            auto sub = this->parseItems(obj["sub-menu"]);
            item = SubMenuItem::create(this, sub);
        }
        else if (obj.count("click")) {
            try {
                item = ActionMenuItem::create(this, obj["click"].as_string());
            } catch(...) {
                items.push_back(this->createError("Invalid \"click\""));
                continue;
            }
        }
        else {
            items.push_back(this->createError("Missing \"click\" or \"sub-menu\""));
            continue;
        }
        if (obj.count("text")) {
            try {
                item->setText(obj["text"].as_string());
                hasTextOrIcon = true;
            } catch(...) {
                items.push_back(this->createError("Invalid \"text\""));
                continue;
            }
        }
        if (obj.count("frame")) {
            try {
                auto spr = obj["frame"].as_string();
                auto frame = CCSprite::createWithSpriteFrameName(spr.c_str());
                if (!frame) {
                    items.push_back(this->createError(fmt::format(
                        "No sprite frame \"{}\"", spr
                    )));
                    continue;
                }
                item->setIcon(frame);
                hasTextOrIcon = true;
            } catch(...) {
                items.push_back(this->createError("Invalid \"frame\""));
                continue;
            }
        }
        items.push_back(item);
    }

    return items;
}

CCNode* ContextMenu::getTarget() const {
    return m_target;
}

void ContextMenu::show(CCPoint const& pos) {
    if (!m_pParent) {
        CCScene::get()->addChild(this);
    }
    this->setZOrder(CCScene::get()->getHighestChildZ() + 100);
    this->setPosition(this->getParent()->convertToWorldSpace(pos));
}

void ContextMenu::hide() {
    this->removeFromParent();
}

$execute {
    new EventListener<AttributeSetFilter>(
        +[](AttributeSetEvent* event) {
            auto node = event->node;
            auto value = event->value;
            if (node->getEventListener("context-menu"_spr)) {
                return;
            }
            node->template addEventListener<MouseEventFilter>(
                "context-menu"_spr,
                [=](MouseEvent* event) {
                    auto click = typeinfo_cast<MouseClickEvent*>(event);
                    if (
                        !click || !click->isDown() ||
                        click->getButton() != MouseButton::Right
                    ) {
                        return MouseResult::Leave;
                    }
                    auto menu = ContextMenu::create(node, value);
                    menu->setID("context-menu"_spr);
                    menu->show(event->getPosition());
                    return MouseResult::Swallow;
                }
            );
        },
        AttributeSetFilter("context-menu"_spr)
    );
    // close all context menus when clicked anywhere
    new EventListener<MouseEventFilter>(
        +[](MouseEvent* event) {
            auto click = typeinfo_cast<MouseClickEvent*>(event);
            if (!click || !click->isDown()) {
                return MouseResult::Leave;
            }
            while (auto old = CCScene::get()->getChildByID("context-menu"_spr)) {
                old->removeFromParent();
            }
            return MouseResult::Leave;
        },
        MouseEventFilter(nullptr)
    );
}
