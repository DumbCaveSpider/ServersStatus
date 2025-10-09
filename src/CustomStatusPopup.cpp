#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include "CustomStatusPopup.hpp"
#include "StatusNode.hpp"
#include <algorithm>
#include <string>
#include <fmt/format.h>
#include <ctime>
#include "StatusStorage.hpp"

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
    if (m_scrollContent)
    {
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

    // Restore nodes from JSON storage
    {
        auto stored = StatusStorage::load();
        m_nodes.clear();
        size_t i = 0;
        for (auto const &s : stored)
        {
            auto name = s.name.empty() ? fmt::format("Custom Status {}", ++i) : s.name;
            auto url = s.url;
            if (auto node = StatusNode::create(name, url, s.id))
            {
                node->setOnDelete([this](StatusNode *n)
                                  {
                    // Remove from storage
                    auto list = StatusStorage::load();
                    StatusStorage::removeById(list, n->getID());
                    StatusStorage::save(list);
                    // Remove from UI
                    m_nodes.erase(std::remove(m_nodes.begin(), m_nodes.end(), n), m_nodes.end());
                    if (n->getParent()) n->removeFromParentAndCleanup(true);
                    refreshLayout(); });
                m_scrollContent->addChild(node);
                m_nodes.push_back(node);
            }
        }
    }

    refreshLayout();
    // Global flag stored inside the JSON is updated on save; nothing to do here

    return true;
}

void CustomStatusPopup::onAdd(CCObject *)
{
    const auto index = m_nodes.size() + 1;
    auto name = std::string("") + std::to_string(index);
    auto url = std::string("");
    auto id = fmt::format("status_{}", geode::utils::string::toLower(std::to_string(time(nullptr))) + std::string("_") + std::to_string(index));

    if (auto node = StatusNode::create(name, url, id))
    {
        node->setOnDelete([this](StatusNode *n)
                          {
            auto list = StatusStorage::load();
            StatusStorage::removeById(list, n->getID());
            StatusStorage::save(list);
            m_nodes.erase(std::remove(m_nodes.begin(), m_nodes.end(), n), m_nodes.end());
            if (n->getParent()) n->removeFromParentAndCleanup(true);
            refreshLayout(); });
        m_scrollContent->addChild(node);
        m_nodes.push_back(node);
        // Persist new node
        auto list = StatusStorage::load();
        StatusStorage::upsertNode(list, StoredNode{id, name, url, false});
        StatusStorage::save(list);
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

    // m_scrollLayer->scrollToTop();
}
