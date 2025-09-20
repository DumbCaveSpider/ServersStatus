#include <Geode/Geode.hpp>
#include <Geode/modify/CCScene.hpp>
#include <Geode/modify/CCMenu.hpp>
#include <Geode/utils/web.hpp>
#include "StatusMonitor.hpp"

using namespace geode::prelude;

bool StatusMonitor::init(){
    if (!CCMenu::init()) return false;

    // get wifi anywhere you go HOLD UP
    // @geode-ignore(unknown-resource)
    m_icon = CCSprite::create("wifiIcon.png"_spr);
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // get settings value
    bool isEnabled = Mod::get()->getSettingValue<bool>("enabled");
    int opacity = Mod::get()->getSettingValue<int>("opacity");
    float scale = Mod::get()->getSettingValue<float>("scale");
    auto position = Mod::get()->getSettingValue<std::string>("position");
    float refresh = Mod::get()->getSettingValue<float>("refresh_rate");

    // position the wifi icon at the top right
    if (position == "Top Right") {
        m_icon->setPosition({winSize.width - 5, winSize.height});
        m_icon->setAnchorPoint({1, 1});
    } else if (position == "Top Left") {
        m_icon->setPosition({5, winSize.height});
        m_icon->setAnchorPoint({0, 1});
    } else if (position == "Bottom Right") {
        m_icon->setPosition({winSize.width - 5, 5});
        m_icon->setAnchorPoint({1, 0});
    } else if (position == "Bottom Left") {
        m_icon->setAnchorPoint({0, 0});
        m_icon->setPosition({5, 5});
    }

    // other icon settings
    m_icon->setScale(scale); // set the icon scale
    m_icon->setOpacity(opacity); // set the icon to be semi-transparent
    m_icon->setColor({ 126, 126, 126 }); // set the icon color to grey
    m_icon->setVisible(isEnabled); // set visibility based on settings
    addChild(m_icon);


    // refresh status every set value
    this->schedule(schedule_selector(StatusMonitor::updateStatus), refresh);

    return true;
}

void StatusMonitor::onEnter() {
    CCMenu::onEnter();
    // fetch settings every time
    bool isEnabled = Mod::get()->getSettingValue<bool>("enabled");
    int opacity = Mod::get()->getSettingValue<int>("opacity");
    float scale = Mod::get()->getSettingValue<float>("scale");
    auto position = Mod::get()->getSettingValue<std::string>("position");

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    // position the wifi icon at the top right
    if (position == "Top Right") {
        m_icon->setPosition({winSize.width - 5, winSize.height - 5});
        m_icon->setAnchorPoint({1, 1});
    } else if (position == "Top Left") {
        m_icon->setPosition({5, winSize.height - 5});
        m_icon->setAnchorPoint({0, 1});
    } else if (position == "Bottom Right") {
        m_icon->setPosition({winSize.width - 5, 5});
        m_icon->setAnchorPoint({1, 0});
    } else if (position == "Bottom Left") {
        m_icon->setAnchorPoint({0, 0});
        m_icon->setPosition({5, 5});
    }
    m_icon->setScale(scale);
    m_icon->setOpacity(opacity);
    m_icon->setVisible(isEnabled);
}

void StatusMonitor::updateStatus(float) {
    checkInternetStatus();
    checkBoomlingsStatus();
    checkGeodeStatus();
}


StatusMonitor* StatusMonitor::create() {
    auto ret = new StatusMonitor();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

void StatusMonitor::checkGeodeStatus() {
    log::debug("checking Geode server status");
    auto lastGeodeCheck = Mod::get()->getSavedValue<std::string>("last_geode_ok");
    geode::utils::web::WebRequest().get("https://api.geode-sdk.org").listen([this, lastGeodeCheck](geode::utils::web::WebResponse* response) {
        if (!response || !response->ok()) {
            log::debug("Geode server offline or unreachable");
            Notification::create(fmt::format("Connection Lost to Geode Server API at {}", lastGeodeCheck), NotificationIcon::Error)->show();
            m_icon->setColor({255, 0, 0}); // red
            return;
        }
        log::debug("Geode server online at {}", lastGeodeCheck);
        m_icon->setColor({0, 255, 0}); // green

    });
}

void StatusMonitor::checkBoomlingsStatus() {
    log::debug("checking Boomlings server status");
    auto lastBoomlingsCheck = Mod::get()->getSavedValue<std::string>("last_boomlings_ok");
    geode::utils::web::WebRequest().get("https://www.boomlings.com").listen([this, lastBoomlingsCheck](geode::utils::web::WebResponse* response) {
        if (!response || !response->ok()) {
            log::debug("Boomlings server offline or unreachable");
            Notification::create(fmt::format("Connection Lost to Boomlings Server at {}", lastBoomlingsCheck), NotificationIcon::Error)->show();
            m_icon->setColor({255, 0, 0}); // red
            return;
        }
        log::debug("Boomlings server online at {}", lastBoomlingsCheck);
        m_icon->setColor({0, 255, 0}); // green
    });
}

void StatusMonitor::checkInternetStatus() {
    log::debug("checking internet status");
    auto lastInternetCheck = Mod::get()->getSavedValue<std::string>("last_internet_ok");
    auto url = Mod::get()->getSettingValue<std::string>("internet_url");
    if (Mod::get()->getSettingValue<bool>("doWeHaveInternet")) {
        if (GameToolbox::doWeHaveInternet()) {
            log::debug("Internet online at {}", lastInternetCheck);
            m_icon->setColor({0, 255, 0}); // green
            return;
        }
        log::debug("Internet offline or unreachable (used GameToolbox::doWeHaveInternet())");
        Notification::create(fmt::format("Connection Lost to Internet at {}", lastInternetCheck), NotificationIcon::Error)->show();
        return;
    }
    geode::utils::web::WebRequest().get(url).listen([this, lastInternetCheck](geode::utils::web::WebResponse* response) {
        if (!response || !response->ok()) {
            log::debug("Internet offline or unreachable");
            Notification::create(fmt::format("Connection Lost to Internet at {}", lastInternetCheck), NotificationIcon::Error)->show();
            m_icon->setColor({255, 0, 0}); // red
            return;
        }
            log::debug("Internet online at {}", lastInternetCheck);
            m_icon->setColor({0, 255, 0}); // green
            return;
    });
}