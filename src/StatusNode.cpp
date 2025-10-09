#include "StatusNode.hpp"
#include <algorithm>
#include <chrono>

using namespace geode::prelude;
using namespace geode::utils;

namespace
{
    const float kNodeWidth = 300.f;
    const float kNodeHeight = 72.f;
    const float kIconOffsetX = 24.f;
    const float kTextOffsetX = 48.f;
    const float kInputWidth = kNodeWidth - kTextOffsetX - 12.f;

    // Sanitize a string for use in a save key: lowercase, allow [a-z0-9_-], replace others with '_'
    std::string sanitizeKeyPart(std::string s)
    {
        using geode::utils::string::toLower;
        toLower(s);
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
                out.push_back(c);
            else if (c == ' ')
                out.push_back('_');
            else
                out.push_back('_');
        }
        // collapse multiple underscores
        std::string collapsed;
        collapsed.reserve(out.size());
        bool lastUnderscore = false;
        for (char c : out)
        {
            if (c == '_')
            {
                if (!lastUnderscore)
                    collapsed.push_back(c);
                lastUnderscore = true;
            }
            else
            {
                collapsed.push_back(c);
                lastUnderscore = false;
            }
        }
        // trim leading/trailing underscores
        size_t start = 0;
        while (start < collapsed.size() && collapsed[start] == '_')
            ++start;
        size_t end = collapsed.size();
        while (end > start && collapsed[end - 1] == '_')
            --end;
        return collapsed.substr(start, end - start);
    }
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

    // Load saved defaults if present using key scheme s_<id>_*
    auto *mod = Mod::get();
    auto keyBase = std::string("s_") + sanitizeKeyPart(m_id);
    auto savedName = mod->getSavedValue<std::string>(keyBase + "_name", m_name);
    auto savedUrl = mod->getSavedValue<std::string>(keyBase + "_url", m_url);
    m_name = savedName;
    m_url = savedUrl;

    // Name input
    m_nameInput = TextInput::create(kInputWidth, "Status name", "bigFont.fnt");
    if (m_nameInput)
    {
        m_nameInput->setCommonFilter(CommonFilter::Any);
        m_nameInput->setMaxCharCount(64);
        m_nameInput->setTextAlign(TextInputAlign::Left);
        m_nameInput->setPosition({kTextOffsetX + kInputWidth / 2.f, kNodeHeight / 2.f + 18.f});
        m_nameInput->setCallback([this](std::string const &value)
                                 {
            m_name = value;
            auto base = std::string("s_") + sanitizeKeyPart(m_id);
            Mod::get()->setSavedValue<std::string>(base + "_name", m_name);
            Mod::get()->setSavedValue<std::string>(base + "_url", m_url); });
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
        m_urlInput->setCallback([this](std::string const &value)
                                {
            m_url = value;
            auto base = std::string("s_") + sanitizeKeyPart(m_id);
            Mod::get()->setSavedValue<std::string>(base + "_name", m_name);
            Mod::get()->setSavedValue<std::string>(base + "_url", m_url);
            this->checkUrlStatus(); });
        this->addChild(m_urlInput, 1);
    }
    this->checkUrlStatus();

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

    // Save current online state persistently
    auto base = std::string("s_") + sanitizeKeyPart(m_id);
    Mod::get()->setSavedValue<bool>(base + "_online", online);
}

void StatusNode::checkUrlStatus()
{
    auto url = m_url;
    if (url.empty())
        return;

    // load last saved state to color immediately
    auto base = std::string("s_") + sanitizeKeyPart(m_id);
    bool last = Mod::get()->getSavedValue<bool>(base + "_online", false);
    this->updateStatusColor(last);

    // Fire off a request; we only need status code
    web::WebRequest()
        .transferBody(false)
        .followRedirects(true)
        .timeout(std::chrono::seconds(5))
        .get(url)
        .then([this](web::WebResponse const &res)
              {
            bool ok = res.ok() && res.code() == 200;
            queueInMainThread([this, ok] {
                this->updateStatusColor(ok);
            }); })
        .expect([](std::string const &err)
                { log::warn("Status check failed: {}", err); });
}
