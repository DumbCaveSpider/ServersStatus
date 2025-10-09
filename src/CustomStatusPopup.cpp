#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include "CustomStatusPopup.hpp"
#include "StatusNode.hpp"
#include <algorithm>
#include <string>
#include <fmt/format.h>
#include <ctime>

using namespace geode::prelude;

CustomStatusPopup *CustomStatusPopup::create()
{
    auto ret = new CustomStatusPopup();
    if (ret && ret->initAnchored(340.f, 260.f))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool CustomStatusPopup::setup()
{
    setTitle("Custom Status");

    auto contentSize = m_mainLayer->getContentSize();
    auto bgScroll = CCScale9Sprite::create("square02_001.png");
    bgScroll->setOpacity(100);
    bgScroll->setContentSize({contentSize.width - 20.f, contentSize.height - 60.f});
    bgScroll->setPosition({contentSize.width / 2.f, contentSize.height / 2.f - 5.f});
    m_mainLayer->addChild(bgScroll);

    // scroll layer
    m_scrollLayer = ScrollLayer::create(bgScroll->getContentSize(), true, true);
    m_scrollLayer->ignoreAnchorPointForPosition(false);
    m_scrollLayer->setAnchorPoint({0.5f, 0.5f});
    m_scrollLayer->setPosition(bgScroll->getPosition());
    m_scrollLayer->setContentSize(bgScroll->getContentSize());
    m_scrollLayer->setID("custom-status-scroll-layer");
    m_mainLayer->addChild(m_scrollLayer);

    m_scrollContent = m_scrollLayer->m_contentLayer;
    if (m_scrollContent) {
        m_scrollContent->ignoreAnchorPointForPosition(false);
        m_scrollContent->setContentSize(bgScroll->getContentSize());
        m_scrollContent->setPosition({0.f, 0.f});
    }

    auto menu = CCMenu::create();
    menu->setPosition({m_mainLayer->getContentSize().width / 2.f, 0.f});
    m_mainLayer->addChild(menu);

    auto addButtonSprite = ButtonSprite::create("Add", "goldFont.fnt", "GJ_button_01.png", 1.25f);

    auto addButton = CCMenuItemSpriteExtra::create(
        addButtonSprite,
        this,
        menu_selector(CustomStatusPopup::onAdd));
    addButton->setID("custom-status-add-button");
    menu->addChild(addButton);

    // Restore nodes from save
    if (auto* mod = Mod::get()) {
        // saved count
        int count = mod->getSavedValue<int>("s_count", 0);
        m_nodes.clear();
        for (int i = 0; i < count; ++i) {
            std::string id = mod->getSavedValue<std::string>(fmt::format("s_node_{}_id", i), "");
            if (id.empty()) continue;
            // Name and URL will be loaded by StatusNode from its own keys
            auto name = mod->getSavedValue<std::string>(std::string("s_") + id + "_name", fmt::format("Custom Status {}", i + 1));
            auto url = mod->getSavedValue<std::string>(std::string("s_") + id + "_url", "https://example.com");
            if (auto node = StatusNode::create(name, url, id)) {
                m_scrollContent->addChild(node);
                m_nodes.push_back(node);
            }
        }
    }

    refreshLayout();

    return true;
}

void CustomStatusPopup::onAdd(CCObject *)
{
    const auto index = m_nodes.size() + 1;
    auto name = std::string("Custom Status ") + std::to_string(index);
    auto url = std::string("https://example.com");
    // Generate a unique stable ID
    auto id = fmt::format("status_{}", geode::utils::string::toLower(std::to_string(time(nullptr))) + std::string("_") + std::to_string(index));

    if (auto node = StatusNode::create(name, url, id))
    {
        m_scrollContent->addChild(node);
        m_nodes.push_back(node);
        // Save list
        if (auto* mod = Mod::get()) {
            mod->setSavedValue<int>("s_count", static_cast<int>(m_nodes.size()));
            for (size_t i = 0; i < m_nodes.size(); ++i) {
                mod->setSavedValue<std::string>(fmt::format("s_node_{}_id", i), m_nodes[i]->getID());
            }
            // Save this node's content under its id (node also saves on change)
            auto base = std::string("s_") + m_nodes.back()->getID();
            mod->setSavedValue<std::string>(base + "_name", m_nodes.back()->getName());
            mod->setSavedValue<std::string>(base + "_url", m_nodes.back()->getUrl());
        }
        refreshLayout();
    }
}

void CustomStatusPopup::refreshLayout()
{
    if (!m_scrollLayer || !m_scrollContent)
    {
        return;
    }

    const float padding = 8.f;
    const auto viewSize = m_scrollLayer->getContentSize();

    float totalHeight = 0.f;
    for (auto node : m_nodes)
    {
        if (!node)
            continue;
        totalHeight += node->getPreferredHeight();
    }
    if (!m_nodes.empty())
    {
        totalHeight += padding * static_cast<float>(m_nodes.size() - 1);
    }
    totalHeight = std::max(totalHeight, viewSize.height);

    m_scrollContent->setContentSize({viewSize.width, totalHeight});

    float cursor = totalHeight;
    for (auto node : m_nodes)
    {
        if (!node)
            continue;
        const float h = node->getPreferredHeight();
        cursor -= h;
        node->setPosition({viewSize.width / 2.f, cursor + h / 2.f});
        cursor -= padding;
    }

    //m_scrollLayer->scrollToTop();
}
