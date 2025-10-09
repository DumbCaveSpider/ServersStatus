#pragma once

#include <Geode/Geode.hpp>
#include <string>

using namespace geode::prelude;

class StatusNode : public CCLayer {
protected:
    bool init(std::string const& name, std::string const& url, std::string const& id);

    std::string m_name;
    std::string m_url;
    std::string m_id;
    TextInput* m_nameInput = nullptr;
    TextInput* m_urlInput = nullptr;
    CCSprite* m_statusIcon = nullptr;
    void updateStatusColor(bool online);
    void checkUrlStatus();

public:
    static StatusNode* create(std::string const& name, std::string const& url, std::string const& id);
    static StatusNode* create(std::string const& name, std::string const& url) { return create(name, url, name); }

    void setStatusIconColor(ccColor3B const& color);
    const std::string& getName() const { return m_name; }
    const std::string& getUrl() const { return m_url; }
    const std::string& getID() const { return m_id; }
    float getPreferredHeight() const { return this->getContentSize().height; }
};
