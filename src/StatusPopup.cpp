#include <Geode/Geode.hpp>
#include <Geode/binding/GameToolbox.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include "StatusPopup.hpp"
#include "StatusMonitor.hpp"

using namespace geode::prelude;

StatusPopup *StatusPopup::create()
{
    auto ret = new StatusPopup();
    if (ret && ret->initAnchored(300.f, 180.f))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool StatusPopup::setup()
{
    setTitle("Servers Status");
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    // user internet
    userInternet = CCLabelBMFont::create("Internet Status: Checking...", "bigFont.fnt");
    userInternet->setColor({100, 100, 100});
    userInternet->setPosition(m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height / 2 + 30);
    userInternet->setScale(0.5f);
    m_mainLayer->addChild(userInternet);
    // subtext timestamp
    {
        auto last = Mod::get()->getSavedValue<std::string>("last_internet_ok");
        std::string text = std::string("Last checked: ") + last;
        auto userInternetTS = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        userInternetTS->setPosition(userInternet->getPositionX(), userInternet->getPositionY() - 15);
        userInternetTS->setScale(0.5f);
        m_mainLayer->addChild(userInternetTS);
    }

    // display boomlings server status
    serverStatus = CCLabelBMFont::create("Boomlings Status: Checking...", "bigFont.fnt");
    serverStatus->setPosition(m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height / 2);
    serverStatus->setColor({100, 100, 100});
    serverStatus->setScale(0.5f);
    m_mainLayer->addChild(serverStatus);
    // subtext timestamp
    {
        auto last = Mod::get()->getSavedValue<std::string>("last_boomlings_ok");
        std::string text = std::string("Last checked: ") + last;
        auto serverStatusTS = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        serverStatusTS->setPosition(serverStatus->getPositionX(), serverStatus->getPositionY() - 15);
        serverStatusTS->setScale(0.5f);
        m_mainLayer->addChild(serverStatusTS);
    }

    // display geode server status
    geodeStatus = CCLabelBMFont::create("GeodeSDK Status: Checking...", "bigFont.fnt");
    geodeStatus->setColor({100, 100, 100});
    geodeStatus->setPosition(m_mainLayer->getContentSize().width / 2, m_mainLayer->getContentSize().height / 2 - 30);
    geodeStatus->setScale(0.5f);
    m_mainLayer->addChild(geodeStatus);
    // subtext timestamp
    {
        auto last = Mod::get()->getSavedValue<std::string>("last_geode_ok");
        std::string text = std::string("Last checked: ") + last;
        auto geodeStatusTS = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        geodeStatusTS->setPosition(geodeStatus->getPositionX(), geodeStatus->getPositionY() - 15);
        geodeStatusTS->setScale(0.5f);
        m_mainLayer->addChild(geodeStatusTS);
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

    // check server status
    checkInternetStatus();
    checkBoomlingsStatus();
    checkGeodeStatus();
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
    geode::utils::web::WebRequest().get(url).listen([this, url](geode::utils::web::WebResponse *response)
                                                    {
        if (!response || !response->ok()) {
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

    geode::utils::web::WebRequest()
        .bodyString(body)
        .post(url)
        .listen([this](geode::utils::web::WebResponse *response)
                {
            if (!response || !response->ok()) {
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
    geode::utils::web::WebRequest().get("https://api.geode-sdk.org").listen([this](geode::utils::web::WebResponse *response)
                                                                            {
        if (!response || !response->ok()) {
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
};