#include "StatusMonitor.hpp"

#include <fmt/chrono.h>

#include <Geode/Geode.hpp>
#include <Geode/modify/CCMenu.hpp>
#include <Geode/modify/CCScene.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <matjson.hpp>
#include <sstream>
#include <string>

#include "Timestamp.hpp"

using namespace geode::prelude;

bool StatusMonitor::init() {
  if (!CCMenu::init())
    return false;

  // get wifi anywhere you go HOLD UP
  // @geode-ignore(unknown-resource)
  m_icon = CCSprite::create("wifiIcon.png"_spr);
  float refresh = Mod::get()->getSettingValue<float>("refresh_rate");
  float padding = Mod::get()->getSettingValue<float>("padding");
  constexpr float kFallbackRefresh = 30.f;

  // Load global custom status OK from JSON storage
  m_custom_ok = true;
  {
    std::ifstream in((Mod::get()->getSaveDir() / "status.json").string(),
                     std::ios::in | std::ios::binary);
    if (in.is_open()) {
      std::ostringstream ss;
      ss << in.rdbuf();
      auto str = ss.str();
      auto json = matjson::parse(str).unwrapOr(matjson::Value());
      m_custom_ok = json["all_online"].asBool().unwrapOr(true);
    }
  }

  m_icon->setColor({100, 100, 100}); // set the icon color to grey
  addChild(m_icon);
  applySettings();

  float interval = refresh > 0.f ? refresh : kFallbackRefresh;
  this->schedule(schedule_selector(StatusMonitor::updateStatus), interval);
  this->updateStatus(0.f);

  // listen for setting changes so UI updates immediately
  m_settingListeners.push_back(geode::listenForSettingChanges<bool>(
      "enabled",
      [this](bool) { geode::queueInMainThread([this]() { applySettings(); }); },
      Mod::get()));
  m_settingListeners.push_back(geode::listenForSettingChanges<int>(
      "opacity",
      [this](int) { geode::queueInMainThread([this]() { applySettings(); }); },
      Mod::get()));
  m_settingListeners.push_back(geode::listenForSettingChanges<float>(
      "scale",
      [this](float) {
        geode::queueInMainThread([this]() { applySettings(); });
      },
      Mod::get()));
  m_settingListeners.push_back(geode::listenForSettingChanges<std::string>(
      "position",
      [this](const std::string &) {
        geode::queueInMainThread([this]() { applySettings(); });
      },
      Mod::get()));
  m_settingListeners.push_back(geode::listenForSettingChanges<float>(
      "padding",
      [this](float) {
        geode::queueInMainThread([this]() { applySettings(); });
      },
      Mod::get()));
  m_settingListeners.push_back(geode::listenForSettingChanges<bool>(
      "disableInLevel",
      [this](bool) { geode::queueInMainThread([this]() { applySettings(); }); },
      Mod::get()));
  m_settingListeners.push_back(geode::listenForSettingChanges<float>(
      "refresh_rate",
      [this](float newRefresh) {
        geode::queueInMainThread([this, newRefresh]() {
          this->unschedule(schedule_selector(StatusMonitor::updateStatus));
          float interval = newRefresh > 0.f ? newRefresh : kFallbackRefresh;
          this->schedule(schedule_selector(StatusMonitor::updateStatus), interval);
          this->updateStatus(0.f);
        });
      },
      Mod::get()));

  return true;
}

void StatusMonitor::onEnter() {
  CCMenu::onEnter();
  applySettings();
}

void StatusMonitor::updateStatus(float) {
  applySettings();
  updateIconColor();

  {
    std::ifstream in((Mod::get()->getSaveDir() / "status.json").string(),
                     std::ios::in | std::ios::binary);
    if (in.is_open()) {
      std::ostringstream ss;
      ss << in.rdbuf();
      auto str = ss.str();
      auto json = matjson::parse(str).unwrapOr(matjson::Value());
      m_custom_ok = json["all_online"].asBool().unwrapOr(true);
    } else {
      m_custom_ok = true;
    }
  }
  checkInternetStatus();
  checkBoomlingsStatus();
  checkGeodeStatus();
  checkArgonStatus();
  updateIconColor();
}

StatusMonitor *StatusMonitor::create() {
  auto ret = new StatusMonitor();
  if (ret && ret->init()) {
    ret->autorelease();
    return ret;
  }
  delete ret;
  return nullptr;
}

void StatusMonitor::updateIconColor() {
  if (!m_icon)
    return;

  // all services down -> red
  if (!m_custom_ok && !m_geode_ok && !m_boomlings_ok && !m_internet_ok &&
      !m_argon_ok) {
    m_icon->setColor({255, 0, 0});
    return;
  }

  // all services up -> green
  if (m_custom_ok && m_geode_ok && m_boomlings_ok && m_internet_ok &&
      m_argon_ok) {
    m_icon->setColor({0, 255, 0});
    return;
  }

  // some services down -> orange
  if (!m_custom_ok || !m_geode_ok || !m_boomlings_ok || !m_internet_ok ||
      !m_argon_ok) {
    m_icon->setColor({255, 165, 0});
    return;
  }

  // default color
  m_icon->setColor({100, 100, 100});
}

void StatusMonitor::applySettings() {
  if (!m_icon)
    return;

  bool isEnabled = Mod::get()->getSettingValue<bool>("enabled");
  int opacity = Mod::get()->getSettingValue<int>("opacity");
  float scale = Mod::get()->getSettingValue<float>("scale");
  auto position = Mod::get()->getSettingValue<std::string>("position");
  float padding = Mod::get()->getSettingValue<float>("padding");

  auto winSize = CCDirector::sharedDirector()->getWinSize();
  // position the wifi icon according to settings
  if (position == "Top Right") {
    m_icon->setPosition(
        {winSize.width - 5 - padding, winSize.height - 5 - padding});
    m_icon->setAnchorPoint({1, 1});
  } else if (position == "Top Left") {
    m_icon->setPosition({5 + padding, winSize.height - 5 - padding});
    m_icon->setAnchorPoint({0, 1});
  } else if (position == "Bottom Right") {
    m_icon->setPosition({winSize.width - 5 - padding, 5 + padding});
    m_icon->setAnchorPoint({1, 0});
  } else if (position == "Bottom Left") {
    m_icon->setAnchorPoint({0, 0});
    m_icon->setPosition({5 + padding, 5 + padding});
  }

  m_icon->setScale(scale);
  m_icon->setOpacity(opacity);
  m_icon->setVisible(isEnabled);

  if (PlayLayer::get()) {
    bool disableInLevel = Mod::get()->getSettingValue<bool>("disableInLevel");
    if (disableInLevel) {
      m_icon->setVisible(false);
    }
  }
  updateIconColor();
}

void StatusMonitor::checkGeodeStatus() {
  log::debug("checking Geode server status");
  auto lastGeodeCheck = Mod::get()->getSavedValue<std::string>("last_geode_ok");
  bool notification = Mod::get()->getSettingValue<bool>("notification");
  geode::async::spawn(
      geode::utils::web::WebRequest().get("https://api.geode-sdk.org"),
      [this, lastGeodeCheck,
       notification](geode::utils::web::WebResponse response) {
        if (!response.ok()) {
          log::debug("GeodeSDK offline or unreachable");
          if (notification) {
            Notification::create(
                fmt::format("Connection Lost to GeodeSDK Server at {}",
                            lastGeodeCheck),
                NotificationIcon::Error)
                ->show();
          }
          m_geode_ok = false;
          geode::queueInMainThread([this]() { this->updateIconColor(); });
          return;
        }
        log::debug("GeodeSDK online at {}", lastGeodeCheck);
        Mod::get()->setSavedValue<std::string>("last_geode_ok",
                                               getLocalTimestamp());
        m_geode_ok = true;
        geode::queueInMainThread([this]() { this->updateIconColor(); });
        return;
      });
}

void StatusMonitor::checkBoomlingsStatus() {
  log::debug("checking Boomlings server status");
  auto lastBoomlingsCheck =
      Mod::get()->getSavedValue<std::string>("last_boomlings_ok");
  bool notification = Mod::get()->getSettingValue<bool>("notification");

  auto url = "http://www.boomlings.com/database/getGJLevels21.php";
  auto body = "type=2&secret=Wmfd2893gb7"; // most liked level

  geode::async::spawn(
      geode::utils::web::WebRequest().bodyString(body).post(url),
      [this, lastBoomlingsCheck,
       notification](geode::utils::web::WebResponse response) {
        if (!response.ok() || response.code() != 200) {
          log::error("Boomlings server offline or unreachable");
          if (notification) {
            Notification::create(
                fmt::format("Connection Lost to Boomlings Server at {}",
                            lastBoomlingsCheck),
                NotificationIcon::Error)
                ->show();
          }
          m_boomlings_ok = false;
          geode::queueInMainThread([this]() { this->updateIconColor(); });
          return;
        }
        log::debug("Boomlings server online at {}", lastBoomlingsCheck);
        Mod::get()->setSavedValue<std::string>("last_boomlings_ok",
                                               getLocalTimestamp());
        m_boomlings_ok = true;
        geode::queueInMainThread([this]() { this->updateIconColor(); });
        return;
      });
}

void StatusMonitor::checkInternetStatus() {
  log::debug("checking internet status");
  auto lastInternetCheck =
      Mod::get()->getSavedValue<std::string>("last_internet_ok");
  bool notification = Mod::get()->getSettingValue<bool>("notification");
  auto url = Mod::get()->getSettingValue<std::string>("internet_url");
  if (Mod::get()->getSettingValue<bool>("doWeHaveInternet")) {
    if (GameToolbox::doWeHaveInternet()) {
      log::debug("{} online at {}", url, lastInternetCheck);
      Mod::get()->setSavedValue<std::string>("last_internet_ok",
                                             getLocalTimestamp());
      m_internet_ok = true;
      updateIconColor();
      return;
    }
    log::error(
        "{} offline or unreachable (used GameToolbox::doWeHaveInternet())",
        url);
    if (notification) {
      Notification::create(
          fmt::format("Connection Lost to Internet at {}", lastInternetCheck),
          NotificationIcon::Error)
          ->show();
    }
    m_internet_ok = false;
    updateIconColor();
    return;
  }
  geode::async::spawn(
      geode::utils::web::WebRequest().get(url),
      [this, lastInternetCheck, url,
       notification](geode::utils::web::WebResponse response) {
        if (!response.ok()) {
          log::error("{} offline or unreachable", url);
          if (notification) {
            Notification::create(
                fmt::format("Connection Lost to Internet at {}",
                            lastInternetCheck),
                NotificationIcon::Error)
                ->show();
          }
          m_internet_ok = false;
          geode::queueInMainThread([this]() { this->updateIconColor(); });
          return;
        }
        log::debug("{} online at {}", url, lastInternetCheck);
        Mod::get()->setSavedValue<std::string>("last_internet_ok",
                                               getLocalTimestamp());
        m_internet_ok = true;
        geode::queueInMainThread([this]() { this->updateIconColor(); });
        return;
      });
}

void StatusMonitor::checkArgonStatus() {
  log::debug("checking Argon server status");
  auto lastArgonCheck = Mod::get()->getSavedValue<std::string>("last_argon_ok");
  bool notification = Mod::get()->getSettingValue<bool>("notification");
  geode::async::spawn(
      geode::utils::web::WebRequest().get("https://argon.globed.dev/"),
      [this, lastArgonCheck,
       notification](geode::utils::web::WebResponse response) {
        if (!response.ok() || response.code() != 200) {
          log::debug("Argon offline or unreachable");
          if (notification) {
            Notification::create(
                fmt::format("Connection Lost to Argon Server at {}",
                            lastArgonCheck),
                NotificationIcon::Error)
                ->show();
          }
          m_argon_ok = false;
          geode::queueInMainThread([this]() { this->updateIconColor(); });
          return;
        }
        log::debug("Argon online at {}", lastArgonCheck);
        Mod::get()->setSavedValue<std::string>("last_argon_ok",
                                               getLocalTimestamp());
        m_argon_ok = true;
        geode::queueInMainThread([this]() { this->updateIconColor(); });
        return;
      });
}