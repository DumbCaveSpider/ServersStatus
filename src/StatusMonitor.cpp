#include <Geode/Geode.hpp>
#include <Geode/modify/CCScene.hpp>
#include <Geode/modify/CCMenu.hpp>
#include <Geode/utils/web.hpp>
#include "StatusMonitor.hpp"
#include <ctime>
#include <fmt/chrono.h>
#include <iomanip>
#include <sstream>
#include <string>

using namespace geode::prelude;

// its time to get the local timestamp
static std::string getLocalTimestamp() {
    auto now = std::time(nullptr);
    return fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(now));
}

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
    m_icon->setColor({ 100, 100, 100 }); // set the icon color to grey
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

    // check if one of them is offline then icon is red
    if (!m_geode_ok || !m_boomlings_ok || !m_internet_ok) {
        m_icon->setColor({ 255, 165, 0 }); // orange if one service is down
    }
    if (m_geode_ok && m_boomlings_ok && m_internet_ok) {
        m_icon->setColor({ 0, 255, 0 }); // green is all services are up
    }
    if (!m_geode_ok && !m_boomlings_ok && !m_internet_ok) {
        m_icon->setColor({ 255, 0, 0 }); // red is all services are down
    }
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
    bool notification = Mod::get()->getSettingValue<bool>("notification");
    geode::utils::web::WebRequest().get("https://api.geode-sdk.org").listen([this, lastGeodeCheck, notification](geode::utils::web::WebResponse* response) {
        if (!response || !response->ok()) {
            log::debug("GeodeSDK offline or unreachable");
            if (notification) {
                Notification::create(fmt::format("Connection Lost to GeodeSDK Server at {}", lastGeodeCheck), NotificationIcon::Error)->show();
            }
            m_geode_ok = false;
            return;
        }
        log::debug("GeodeSDK online at {}", lastGeodeCheck);
        Mod::get()->setSavedValue<std::string>("last_geode_ok", getLocalTimestamp());
        m_geode_ok = true;
        return;
    });
}

void StatusMonitor::checkBoomlingsStatus() {
    log::debug("checking Boomlings server status");
    auto lastBoomlingsCheck = Mod::get()->getSavedValue<std::string>("last_boomlings_ok");
    bool notification = Mod::get()->getSettingValue<bool>("notification");
    geode::utils::web::WebRequest().get("https://www.boomlings.com").listen([this, lastBoomlingsCheck, notification](geode::utils::web::WebResponse* response) {
        if (!response || !response->ok()) {
            log::error("Boomlings server offline or unreachable");
            if (notification) {
                Notification::create(fmt::format("Connection Lost to Boomlings Server at {}", lastBoomlingsCheck), NotificationIcon::Error)->show();
            }
            m_boomlings_ok = false;
            return;
        }
        log::debug("Boomlings server online at {}", lastBoomlingsCheck);
        Mod::get()->setSavedValue<std::string>("last_boomlings_ok", getLocalTimestamp());
        m_boomlings_ok = true;
        return;
    });
}

void StatusMonitor::checkInternetStatus() {
    log::debug("checking internet status");
    auto lastInternetCheck = Mod::get()->getSavedValue<std::string>("last_internet_ok");
    bool notification = Mod::get()->getSettingValue<bool>("notification");
    auto url = Mod::get()->getSettingValue<std::string>("internet_url");
    if (Mod::get()->getSettingValue<bool>("doWeHaveInternet")) {
        if (GameToolbox::doWeHaveInternet()) {
            log::debug("{} online at {}", url, lastInternetCheck);
            Mod::get()->setSavedValue<std::string>("last_internet_ok", getLocalTimestamp());
            m_internet_ok = true;
            return;
        }
        log::error("{} offline or unreachable (used GameToolbox::doWeHaveInternet())", url);
        if (notification) {
            Notification::create(fmt::format("Connection Lost to Internet at {}", lastInternetCheck), NotificationIcon::Error)->show();
        }
        m_internet_ok = false;
        return;
    }
    geode::utils::web::WebRequest().get(url).listen([this, lastInternetCheck, url, notification](geode::utils::web::WebResponse* response) {
        if (!response || !response->ok()) {
            log::error("{} offline or unreachable", url);
            if (notification) {
                Notification::create(fmt::format("Connection Lost to Internet at {}", lastInternetCheck), NotificationIcon::Error)->show();
            }
            m_internet_ok = false;
            return;
        }
        log::debug("{} online at {}", url, lastInternetCheck);
        Mod::get()->setSavedValue<std::string>("last_internet_ok", getLocalTimestamp());
        m_internet_ok = true;
        return;
    });
}