#include "StatusNode.hpp"
#include <algorithm>
#include <chrono>
#include <string>
#include <ctime>
#include "Timestamp.hpp"
#include <fmt/format.h>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <regex>
#include "StatusStorage.hpp"

using namespace geode::prelude;
using namespace geode::utils;

namespace
{
    const float kNodeWidth = 320.f;
    const float kNodeHeight = 90.f;
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
    delete ret;
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

    if (auto bg = CCSprite::create())
    {
        bg->setTextureRect(CCRectMake(0, 0, kNodeWidth, kNodeHeight));
        bg->setColor({194, 114, 62});
        bg->setAnchorPoint({0.5f, 0.5f});
        bg->setPosition({kNodeWidth / 2.f, kNodeHeight / 2.f});
        this->addChild(bg);
        m_bg = bg;
    }

    // Status icon
    m_statusIcon = CCSprite::create("wifiIcon.png"_spr);
    if (m_statusIcon)
    {
        m_statusIcon->setAnchorPoint({0.5f, 0.5f});
        m_statusIcon->setPosition({kIconOffsetX, kNodeHeight / 2.f + 10.f});
        m_statusIcon->setScale(0.6f);
        this->addChild(m_statusIcon, 1);

        m_statusCodeLabel = CCLabelBMFont::create("Status Code\n-", "chatFont.fnt");
        m_statusCodeLabel->setScale(0.5f);
        m_statusCodeLabel->setPosition({kIconOffsetX, kNodeHeight / 2.f - 20.f});
        m_statusCodeLabel->setAlignment(kCCTextAlignmentCenter);
        this->addChild(m_statusCodeLabel, 1);
    }

    // Load saved defaults from JSON storage
    if (auto sn = StatusStorage::getById(StatusStorage::load(), m_id))
    {
        m_name = sn->name;
        m_url = sn->url;
        if (!sn->last_ping.empty()) {
            m_lastPingTimestamp = sn->last_ping;
        }
    }

    float refresh = Mod::get()->getSettingValue<float>("refresh_rate");

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

        m_lastPingLabel = CCLabelBMFont::create("Last ping: -", "chatFont.fnt");
        m_lastPingLabel->setScale(0.5f);
        m_lastPingLabel->setPosition({kTextOffsetX + kInputWidth / 2.f, kNodeHeight / 2.f - 38.f});
        m_lastPingLabel->setAlignment(kCCTextAlignmentCenter);
        if (!m_lastPingTimestamp.empty()) {
            m_lastPingLabel->setString((std::string("Last ping: ") + m_lastPingTimestamp).c_str());
        }
        this->addChild(m_lastPingLabel, 1);
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
            CircleBaseColor::Green,
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
    m_bg->setColor(online ? ccColor3B{100, 200, 100} : ccColor3B{200, 100, 100});

    // Save current online state persistently via JSON storage
    auto nodes = StatusStorage::load();
    std::string lastPing;
    if (auto ex = StatusStorage::getById(nodes, m_id)) {
        lastPing = ex->last_ping;
    } else {
        lastPing = m_lastPingTimestamp;
    }
    StatusStorage::upsertNode(nodes, StoredNode{m_id, m_name, m_url, online, lastPing});
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
        m_bg->setColor({230, 150, 10});
    }

    if (useLastSaved)
    {
        bool last = false;
        if (auto sn = StatusStorage::getById(StatusStorage::load(), m_id))
            last = sn->online;
        this->updateStatusColor(last);
    }

    // Cancel any existing request
    m_requestTask.cancel();

    if (m_statusCodeLabel) m_statusCodeLabel->setString("Status Code\n-");
    if (m_lastPingLabel) m_lastPingLabel->setString("Last ping: pending");

    // status code
    bool notify = !useLastSaved;

    m_requestTask.spawn(
        web::WebRequest()
            .transferBody(false)
            .followRedirects(true)
            .timeout(std::chrono::seconds(5))
            .get(url),
        [this, notify](geode::utils::web::WebResponse res) {
            bool ok = res.ok() && res.code() == 200;
            int code = res.code();
            std::string codeText = code ? ("Status Code\n" + std::to_string(code)) : std::string("Status Code\n-");

            std::string timestamp = getLocalTimestamp();
            std::string timeText = std::string("Last ping: ") + timestamp;

            if (ok) {
                auto nodes = StatusStorage::load();
                if (auto ex = StatusStorage::getById(nodes, m_id)) {
                    StoredNode copy = *ex;
                    copy.online = true;
                    copy.last_ping = timestamp;
                    StatusStorage::upsertNode(nodes, copy);
                } else {
                    StatusStorage::upsertNode(nodes, StoredNode{m_id, m_name, m_url, true, timestamp});
                }
                StatusStorage::save(nodes);
            }

            std::string notifyFmt = ok ? "Ping successful ({})" : "Ping failed ({})";
            std::string notifyMsg = fmt::format(fmt::runtime(notifyFmt), code);

            queueInMainThread([this, ok, notify, codeText, timeText, timestamp, notifyMsg]
                              {
                this->updateStatusColor(ok);
                if (m_statusCodeLabel) m_statusCodeLabel->setString(codeText.c_str());
                if (ok) {
                    if (m_lastPingLabel) m_lastPingLabel->setString(timeText.c_str());
                    this->m_lastPingTimestamp = timestamp;
                }
                if (notify) {
                    Notification::create(notifyMsg, ok ? NotificationIcon::Success : NotificationIcon::Error)->show();
                } });
        });
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
    geode::createQuickPopup(
        "Delete Status",
        "Are you sure you want to delete this custom status?",
        "Cancel",
        "Delete",
        320.f,
        [this](FLAlertLayer* layer, bool btn1) {
            if (btn1) {
                if (m_onDelete)
                    m_onDelete(this);
            }
        },
        true,
        true
    );
}

void StatusNode::onExit()
{
    // Ensure any pending request is cancelled to avoid callbacks after node is gone
    m_requestTask.cancel();
    CCLayer::onExit();
} 
