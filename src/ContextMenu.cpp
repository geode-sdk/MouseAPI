#include "../include/ContextMenu.hpp"

using namespace geode::prelude;
using namespace mouse;

bool ContextMenuItem::init() {
    if (!CCNode::init())
        return false;
    
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
    this->insertBefore(icon, nullptr);
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

bool SubMenuItem::init(std::vector<ItemRef> const& items) {
    if (!ContextMenuItem::init())
        return false;

    m_items = items;

    return true;
}

SubMenuItem* SubMenuItem::create(std::vector<ItemRef> const& items) {
    auto ret = new SubMenuItem;
    if (ret && ret->init(items)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void SubMenuItem::select() {
    if (!m_menu) {
        auto bbox = this->boundingBox();
        m_menu = ContextMenu::create(m_items);
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

bool ContextMenu::init(std::vector<ItemRef> const& items) {
    if (!CCNode::init())
        return false;
    
    m_items = items;

    auto bg = CCScale9Sprite::create("square02b_small-uhd.png", { 0, 0, 40, 40 });
    bg->setColor({ 0, 0, 0 });
    bg->setOpacity(155);
    bg->setScale(.4f);
    this->addChild(bg);

    float width = 0.f;
    for (auto& item : items) {
        auto w = item->getPreferredWidth();
        if (w > width) {
            width = w;
        }
    }
    width = clamp(width, 100.f, 160.f);

    m_container = CCNode::create();
    for (auto& item : items) {
        m_container->addChild(item);
        item->fitToWidth(width);
        item->updateLayout();
    }
    auto layout = RowLayout::create()
        ->setGrowCrossAxis(true)
        ->setAxisAlignment(AxisAlignment::Even)
        ->setGap(0.f);
    
    auto containerHeight = layout->getSizeHint(m_container).height;

    m_container->setContentSize({ width, containerHeight });
    m_container->setLayout(layout);
    this->addChild(m_container);

    auto height = clamp(containerHeight, 0.f, 200.f);

    this->setContentSize({ width, height });
    m_container->setPosition(m_obContentSize / 2);
    bg->setContentSize(m_obContentSize / bg->getScale());
    bg->setPosition(m_obContentSize / 2);

    return true;
}

ContextMenu* ContextMenu::create(std::vector<ItemRef> const& items) {
    auto ret = new ContextMenu;
    if (ret && ret->init(items)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

ContextMenu* ContextMenu::get(bool create) {
    if (!s_current && create) {
        s_current = ContextMenu::create({});
    }
    return s_current;
}

void ContextMenu::show(CCPoint const& pos) {
    if (!m_pParent) {
        CCScene::get()->addChild(this);
    }
    this->setZOrder(CCScene::get()->getHighestChildZ() + 100);
    this->setPosition(
        this->getParent()->convertToWorldSpace(pos) + 
            this->getScaledContentSize() * this->getAnchorPoint()
    );
}

void ContextMenu::hide() {
    if (s_current == this) {
        s_current = nullptr;
    }
    this->removeFromParent();
}

$execute {
    new EventListener<AttributeSetFilter>(
        +[](AttributeSetEvent* event) {
            auto node = event->node;
            if (!node->getEventListener("context-menu"_spr)) {
                node->template addEventListener<MouseEventFilter>(
                    "context-menu"_spr,
                    [=](MouseEvent* event) {
                        if (auto click = typeinfo_cast<MouseClickEvent*>(event)) {
                            if (click->getButton() == MouseButton::Right) {

                            }
                        }
                        auto tip = static_cast<Tooltip*>(CCScene::get()->getChildByID("tooltip"_spr));
                        if (MouseAttributes::from(node)->isHovered()) {
                            if (tip) {
                                tip->move(event->getPosition() + ccp(5.f, 0.));
                            }
                            else {
                                if (auto value = node->template getAttribute<std::string>("tooltip"_spr)) {
                                    Tooltip::create(value.value())->show(event->getPosition() + ccp(5.f, 0.));
                                }
                            }
                        }
                        else if (tip) {
                            tip->hide();
                        }
                        return MouseResult::Eat;
                    }
                );
            }
        },
        AttributeSetFilter("context-menu"_spr)
    );
}
