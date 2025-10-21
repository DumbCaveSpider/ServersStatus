#include "StatusNode.hpp"
#include <algorithm>
#include <chrono>
#include <Geode/utils/web.hpp>
#include <regex>
#include "StatusStorage.hpp"

using namespace geode::prelude;
using namespace geode::utils;

namespace
{
    const float kNodeWidth = 315.f;
    const float kNodeHeight = 70.f;
    const float kIconOffsetX = 24.f;
    const float kTextOffsetX = 50.f;
    const float kInputWidth = kNodeWidth - kTextOffsetX - 54.f;

}

StatusNode *StatusNode::create(std::string const &name, std::string const &url, std::string const &id)
{
    auto ret = new StatusNode();
    if (ret && ret->init(name, url, id))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool StatusNode::init(std::string const &name, std::string const &url, std::string const &id)
{
    if (!CCLayer::init())
    {
        return false;
    }

    this->ignoreAnchorPointForPosition(false);
    this->setAnchorPoint({0.5f, 0.5f});
    this->setContentSize({kNodeWidth, kNodeHeight});

    m_name = name;
    m_url = url;
    m_id = id;

    // Background panel
    if (auto bg = CCScale9Sprite::create("GJ_square05.png"))
    {
        bg->setContentSize({kNodeWidth, kNodeHeight});
        bg->setOpacity(75);
        bg->setAnchorPoint({0.5f, 0.5f});
        bg->setPosition({kNodeWidth / 2.f, kNodeHeight / 2.f});
        this->addChild(bg);
    }

    // Status icon
    m_statusIcon = CCSprite::create("wifiIcon.png"_spr);
    if (m_statusIcon)
    {
        m_statusIcon->setAnchorPoint({0.5f, 0.5f});
        m_statusIcon->setPosition({kIconOffsetX, kNodeHeight / 2.f});
        m_statusIcon->setScale(0.6f);
        this->addChild(m_statusIcon, 1);
    }

    // Load saved defaults from JSON storage
    if (auto sn = StatusStorage::getById(StatusStorage::load(), m_id))
    {
        m_name = sn->name;
        m_url = sn->url;
    }

    float refresh = Mod::get()->getSettingValue<float>("refresh_rate");

    // m_name/m_url were possibly overridden from storage above

    // Name input
    m_nameInput = TextInput::create(kInputWidth, "Status name", "bigFont.fnt");
    if (m_nameInput)
    {
        m_nameInput->setCommonFilter(CommonFilter::Any);
        m_nameInput->setMaxCharCount(64);
        m_nameInput->setTextAlign(TextInputAlign::Left);
        m_nameInput->setPosition({kTextOffsetX + kInputWidth / 2.f, kNodeHeight / 2.f + 16.f});
        m_nameInput->setString(m_name);
        m_nameInput->setCallback([this](std::string const &value)
                                 {
            m_name = value;
            auto nodes = StatusStorage::load();
            bool online = false;
            if (auto ex = StatusStorage::getById(nodes, m_id)) online = ex->online;
            StatusStorage::upsertNode(nodes, StoredNode{m_id, m_name, m_url, online});
            StatusStorage::save(nodes); });
        this->addChild(m_nameInput, 1);
    }

    // URL input
    m_urlInput = TextInput::create(kInputWidth, "Status URL", "chatFont.fnt");
    if (m_urlInput)
    {
        m_urlInput->setCommonFilter(CommonFilter::Any);
        m_urlInput->setMaxCharCount(256);
        m_urlInput->setTextAlign(TextInputAlign::Left);
        m_urlInput->setPosition({kTextOffsetX + kInputWidth / 2.f, kNodeHeight / 2.f - 16.f});
        m_urlInput->setString(m_url);
        m_urlInput->setCallback([this](std::string const &value)
                                {
            m_url = value;
            auto nodes = StatusStorage::load();
            bool online = false;
            if (auto ex = StatusStorage::getById(nodes, m_id)) online = ex->online;
            StatusStorage::upsertNode(nodes, StoredNode{m_id, m_name, m_url, online});
            StatusStorage::save(nodes);
            // Validate URL as user types; only notify once per invalid state
            static const std::regex urlRe(R"((http|https):\/\/([\w_-]+(?:(?:\.[\w_-]+)+))([\w.,@?^=%&:\/~+#-]*[\w@?^=%&\/~+#-]))");
            bool valid = std::regex_match(m_url, urlRe);
            if (!valid) {
                if (!m_urlInvalidNotified) {
                    Notification::create("Invalid URL format", NotificationIcon::Error)->show();
                    m_urlInvalidNotified = true;
                }
            } else {
                m_urlInvalidNotified = false;
            } });
        this->addChild(m_urlInput, 1);
    }
    // Buttons on the right: Ping and Delete
    if (auto menu = CCMenu::create())
    {
        menu->setAnchorPoint({0.f, 0.f});
        menu->setPosition({kNodeWidth - 22.f, 0.f});
        this->addChild(menu, 2);
        auto pingSpr = CircleButtonSprite::createWithSprite(
            "ping.png"_spr,
            0.75f,
            CircleBaseColor::Green,
            CircleBaseSize::SmallAlt);
        auto delSpr = CircleButtonSprite::createWithSprite(
            "delete.png"_spr,
            0.75f,
            CircleBaseColor::Pink,
            CircleBaseSize::SmallAlt);

        pingSpr->setScale(0.75f);
        delSpr->setScale(0.75f);
        auto pingBtn = CCMenuItemSpriteExtra::create(pingSpr, this, menu_selector(StatusNode::onPingPressed));
        auto delBtn = CCMenuItemSpriteExtra::create(delSpr, this, menu_selector(StatusNode::onDeletePressed));

        pingBtn->setPosition({0.f, kNodeHeight / 2.f + 18.f});
        delBtn->setPosition({0.f, kNodeHeight / 2.f - 16.f});

        menu->addChild(pingBtn);
        menu->addChild(delBtn);
    }
    this->checkUrlStatus(true);

    if (refresh > 0.f)
    {
        this->schedule(schedule_selector(StatusNode::updateStatusTimer), refresh);
    }

    return true;
}

void StatusNode::setStatusIconColor(ccColor3B const &color)
{
    if (m_statusIcon)
    {
        m_statusIcon->setColor(color);
    }
}

void StatusNode::updateStatusColor(bool online)
{
    if (!m_statusIcon)
        return;
    const ccColor3B green{50, 220, 90};
    const ccColor3B red{220, 60, 60};
    m_statusIcon->setColor(online ? green : red);

    // Save current online state persistently via JSON storage
    auto nodes = StatusStorage::load();
    StatusStorage::upsertNode(nodes, StoredNode{m_id, m_name, m_url, online});
    StatusStorage::save(nodes);
}

void StatusNode::checkUrlStatus(bool useLastSaved)
{
    auto url = m_url;
    if (url.empty())
        return;

    static const std::regex urlRe(R"((http|https):\/\/([\w_-]+(?:(?:\.[\w_-]+)+))([\w.,@?^=%&:\/~+#-]*[\w@?^=%&\/~+#-]))");
    if (!std::regex_match(url, urlRe))
    {
        Notification::create("Invalid URL format", NotificationIcon::Error)->show();
        m_urlInvalidNotified = true;
        return;
    }

    if (!useLastSaved)
    {
        m_statusIcon->setColor({255, 255, 255});
    }

    if (useLastSaved)
    {
        bool last = false;
        if (auto sn = StatusStorage::getById(StatusStorage::load(), m_id))
            last = sn->online;
        this->updateStatusColor(last);
    }

    // Cancel request
    if (!m_requestTask.isNull() && m_requestTask.isPending())
    {
        m_requestTask.cancel();
    }

    // status code
    m_requestTask = web::WebRequest()
                        .transferBody(false)
                        .followRedirects(true)
                        .timeout(std::chrono::seconds(5))
                        .get(url);

    bool notify = !useLastSaved;

    m_requestTask.listen(
        [this, notify](web::WebResponse const *res)
        {
            bool ok = res && res->ok() && res->code() == 200;
            queueInMainThread([this, ok, notify]
                              {
                this->updateStatusColor(ok);
                if (notify) {
                    Notification::create(ok ? "Ping successful" : "Ping failed", ok ? NotificationIcon::Success : NotificationIcon::Error)->show();
                } });
        },
        [](web::WebProgress const *) {},
        [] {});
}

void StatusNode::updateStatusTimer(float)
{
    this->checkUrlStatus(true);
}

void StatusNode::onPingPressed(CCObject *)
{
    this->checkUrlStatus(false);
}

void StatusNode::onDeletePressed(CCObject *)
{
    if (m_onDelete)
        m_onDelete(this);
}

void StatusNode::onExit()
{
    // Ensure any pending request is cancelled to avoid callbacks after node is gone
    if (!m_requestTask.isNull() && m_requestTask.isPending())
    {
        m_requestTask.cancel();
    }
    CCLayer::onExit();
}
