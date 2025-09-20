#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "StatusPopup.hpp"
#include "StatusMonitor.hpp"

using namespace geode::prelude;

class $modify(StatusMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        // get the close-menu 
        auto closeMenu = this->getChildByID("close-menu");
        auto wifiIcon = CCSprite::create("wifiIcon.png"_spr);
        auto scene = CCDirector::sharedDirector()->getRunningScene();

        auto statusButton = CCMenuItemSpriteExtra::create(
            CircleButtonSprite::create(
                wifiIcon,
                CircleBaseColor::Green,
                CircleBaseSize::Small
            ),
            this,
            menu_selector(StatusMenuLayer::onStatusButton)
        );

        statusButton->setScale(0.4f);

        closeMenu->addChild(statusButton);
        closeMenu->updateLayout();

        // create StatusMonitor if it doesn't exist and keep it across scenes
        if (!scene->getChildByID("status-monitor")) {
            auto monitor = StatusMonitor::create();
            monitor->setID("status-monitor");
            monitor->setPosition({0, 0});
            this->addChild(monitor, 1000); // high z-order to be on top
            geode::SceneManager::get()->keepAcrossScenes(monitor);
        }

        return true;
    }
    void onEnter() {
        MenuLayer::onEnter();
    }
    void onStatusButton(CCObject*) {
        if (auto popup = StatusPopup::create()) {
            popup->show();
        }
    }
};