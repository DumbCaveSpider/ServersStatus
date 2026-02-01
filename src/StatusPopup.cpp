#include <Geode/Geode.hpp>
#include <Geode/binding/GameToolbox.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include "StatusPopup.hpp"
#include "StatusMonitor.hpp"
#include "CustomStatusPopup.hpp"

using namespace geode::prelude;

StatusPopup *StatusPopup::create()
{
    auto ret = new StatusPopup();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool StatusPopup::init()
{
    if (!Popup::init(300.f, 200.f)) return false;

    setTitle("Servers Status");
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // evenly space the status labels
    const float width = m_mainLayer->getContentSize().width;
    const float height = m_mainLayer->getContentSize().height;
    const float centerX = width / 2.0f;
    const float centerY = height / 2.0f;

    // number of main status lines and spacing between them
    const int lines = 4;
    const float spacing = 30.0f;
    // top-most label Y (so labels are centered vertically as a group)
    const float topY = centerY + spacing * (lines - 1) / 2.0f;

    // Internet
    userInternet = CCLabelBMFont::create("Internet Status: Checking...", "bigFont.fnt");
    userInternet->setColor({100, 100, 100});
    userInternet->setScale(0.5f);
    userInternet->setPosition({centerX, topY});
    m_mainLayer->addChild(userInternet);
    // timestamp under Internet
    {
        auto last = Mod::get()->getSavedValue<std::string>("last_internet_ok");
        std::string text = std::string("Last checked: ") + last;
        auto lbl = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        lbl->setScale(0.5f);
        lbl->setPosition({centerX, topY - 15});
        m_mainLayer->addChild(lbl);
    }

    // Boomlings (next line)
    serverStatus = CCLabelBMFont::create("Boomlings Status: Checking...", "bigFont.fnt");
    serverStatus->setColor({100, 100, 100});
    serverStatus->setScale(0.5f);
    serverStatus->setPosition({centerX, topY - spacing});
    m_mainLayer->addChild(serverStatus);
    // timestamp under Boomlings
    {
        auto last = Mod::get()->getSavedValue<std::string>("last_boomlings_ok");
        std::string text = std::string("Last checked: ") + last;
        auto lbl = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        lbl->setScale(0.5f);
        lbl->setPosition({centerX, topY - spacing - 15});
        m_mainLayer->addChild(lbl);
    }

    // GeodeSDK (next line)
    geodeStatus = CCLabelBMFont::create("GeodeSDK Status: Checking...", "bigFont.fnt");
    geodeStatus->setColor({100, 100, 100});
    geodeStatus->setScale(0.5f);
    geodeStatus->setPosition({centerX, topY - spacing * 2});
    m_mainLayer->addChild(geodeStatus);
    // timestamp under Geode
    {
        auto last = Mod::get()->getSavedValue<std::string>("last_geode_ok");
        std::string text = std::string("Last checked: ") + last;
        auto lbl = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        lbl->setScale(0.5f);
        lbl->setPosition({centerX, topY - spacing * 2 - 15});
        m_mainLayer->addChild(lbl);
    }

    // Argon (next line)
    argonStatus = CCLabelBMFont::create("Argon Status: Checking...", "bigFont.fnt");
    argonStatus->setColor({100, 100, 100});
    argonStatus->setScale(0.5f);
    argonStatus->setPosition({centerX, topY - spacing * 3});
    m_mainLayer->addChild(argonStatus);
    // timestamp under Argon
    {
        auto last = Mod::get()->getSavedValue<std::string>("last_argon_ok");
        std::string text = std::string("Last checked: ") + last;
        auto lbl = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        lbl->setScale(0.5f);
        lbl->setPosition({centerX, topY - spacing * 3 - 15});
        m_mainLayer->addChild(lbl);
    }

    // mod settings button
    auto modSettingsMenu = CCMenu::create();
    modSettingsMenu->setPosition({0, 0});
    auto modSettingsBtnSprite = CircleButtonSprite::createWithSpriteFrameName(
        // @geode-ignore(unknown-resource)
        "geode.loader/settings.png",
        1.f,
        CircleBaseColor::Green,
        CircleBaseSize::Medium);
    modSettingsBtnSprite->setScale(0.75f);

    auto modSettingsButton = CCMenuItemSpriteExtra::create(
        modSettingsBtnSprite,
        this,
        menu_selector(StatusPopup::onModSettings));
    modSettingsMenu->addChild(modSettingsButton);
    m_mainLayer->addChild(modSettingsMenu);

    // button to open the custom status popup
    auto customMenu = CCMenu::create();
    customMenu->setPosition({centerX, 0.f});
    m_mainLayer->addChild(customMenu, 5);

    auto customButtonSprite = ButtonSprite::create("Show Custom");

    auto customButton = CCMenuItemSpriteExtra::create(
        customButtonSprite,
        this,
        menu_selector(StatusPopup::onOpenCustomStatus));
    customButton->setID("status-popup-open-custom");
    customMenu->addChild(customButton);

    // check server status
    checkInternetStatus();
    checkBoomlingsStatus();
    checkGeodeStatus();
    checkArgonStatus();
    // immediate status refresh on the existing monitor instance
    if (auto scene = CCDirector::sharedDirector()->getRunningScene())
    {
        if (auto node = scene->getChildByID("status-monitor"))
        {
            if (auto monitor = typeinfo_cast<StatusMonitor *>(node))
            {
                monitor->updateStatus(0.0f);
            }
        }
    }

    return true;
}

void StatusPopup::onModSettings(CCObject *sender)
{
    openSettingsPopup(getMod());
}

void StatusPopup::checkInternetStatus()
{
    log::debug("checking internet status");
    // do we have internet?
    if (Mod::get()->getSettingValue<bool>("doWeHaveInternet"))
    {
        if (GameToolbox::doWeHaveInternet())
        {
            userInternet->setString("Internet Status: Online");
            userInternet->setColor({0, 255, 0});
            return;
        }
        log::debug("Internet offline or unreachable");
        userInternet->setString("Internet Status: Offline");
        userInternet->setColor({255, 0, 0});
        return;
    }
    auto url = Mod::get()->getSettingValue<std::string>("internet_url");
    m_internetTask.cancel();
    m_internetTask.spawn(
        geode::utils::web::WebRequest().get(url),
        [this, url](geode::utils::web::WebResponse response) {
            if (!response.ok()) {
                log::debug("{} offline or unreachable", url);
                if (userInternet) {
                    userInternet->setString("Internet Status: Offline");
                    userInternet->setColor({ 255, 0, 0 });
                }
                return;
            }
            log::debug("{} online", url);
            if (userInternet) {
                userInternet->setString("Internet Status: Online");
                userInternet->setColor({ 0, 255, 0 });
            } });
}

void StatusPopup::checkBoomlingsStatus()
{
    log::debug("checking Boomlings server status");
    auto url = "http://www.boomlings.com/database/getGJLevels21.php";
    auto body = "type=2&secret=Wmfd2893gb7"; // most liked level

    m_boomlingsTask.cancel();
    m_boomlingsTask.spawn(
        geode::utils::web::WebRequest()
            .bodyString(body)
            .post(url),
        [this](geode::utils::web::WebResponse response) {
            if (!response.ok() || response.code() != 200) {
                log::debug("Boomlings server offline or unreachable");
                if (serverStatus) {
                    serverStatus->setString("Boomlings Status: Offline");
                    serverStatus->setColor({255, 0, 0});
                }
                return;
            }
            log::debug("Boomlings server online");
            if (serverStatus) {
                serverStatus->setString("Boomlings Status: Online");
                serverStatus->setColor({0, 255, 0});
            } });
}

void StatusPopup::checkGeodeStatus()
{
    log::debug("checking GeodeSDK status");
    m_geodeTask.cancel();
    m_geodeTask.spawn(
        geode::utils::web::WebRequest().get("https://api.geode-sdk.org"),
        [this](geode::utils::web::WebResponse response) {
        if (!response.ok()) {
            log::debug("GeodeSDK offline or unreachable");
            if (geodeStatus) {
                geodeStatus->setString("GeodeSDK Status: Offline");
                geodeStatus->setColor({ 255, 0, 0 });
            }
            return;
        }
        log::debug("GeodeSDK online");
        if (geodeStatus) {
            geodeStatus->setString("GeodeSDK Status: Online");
            geodeStatus->setColor({ 0, 255, 0 });
        } });
}

void StatusPopup::checkArgonStatus()
{
    log::debug("checking Argon server status");
    auto url = "https://argon.globed.dev/";

    m_argonTask.cancel();
    m_argonTask.spawn(
        geode::utils::web::WebRequest().get(url),
        [this, url](geode::utils::web::WebResponse response) {
                    if (!response.ok() || response.code() != 200) {
                        log::debug("Argon server offline or unreachable");
                        if (argonStatus) {
                            argonStatus->setString("Argon Status: Offline");
                            argonStatus->setColor({255, 0, 0});
                        }
                        return;
                    }
                    log::debug("Argon server online");
                    if (argonStatus) {
                        argonStatus->setString("Argon Status: Online");
                        argonStatus->setColor({0, 255, 0});
                    } });
}

void StatusPopup::onOpenCustomStatus(CCObject *)
{
    if (auto popup = CustomStatusPopup::create())
    {
        popup->show();
    }
}