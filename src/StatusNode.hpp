#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <functional>

using namespace geode::prelude;
using namespace geode::utils;

class StatusNode : public CCLayer
{
protected:
    bool init(std::string const &name, std::string const &url, std::string const &id);
    void onExit() override;
    void onDeletePressed(CCObject *);
    void onPingPressed(CCObject *);
    void updateStatusTimer(float);

    std::string m_name;
    std::string m_url;
    std::string m_id;
    TextInput *m_nameInput = nullptr;
    TextInput *m_urlInput = nullptr;
    CCSprite *m_statusIcon = nullptr;
    web::WebTask m_requestTask;
    std::function<void(StatusNode *)> m_onDelete;
    bool m_urlInvalidNotified = false;
    void updateStatusColor(bool online);
    void checkUrlStatus(bool useLastSaved = true);

public:
    static StatusNode *create(std::string const &name, std::string const &url, std::string const &id);
    static StatusNode *create(std::string const &name, std::string const &url) { return create(name, url, name); }

    void setStatusIconColor(ccColor3B const &color);
    void setOnDelete(std::function<void(StatusNode *)> cb) { m_onDelete = std::move(cb); }
    const std::string &getName() const { return m_name; }
    const std::string &getUrl() const { return m_url; }
    const std::string &getID() const { return m_id; }
    float getPreferredHeight() const { return this->getContentSize().height; }
};
