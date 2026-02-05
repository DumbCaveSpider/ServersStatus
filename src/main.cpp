#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCLayer.hpp>
#include "Geode/ui/OverlayManager.hpp"
#include "StatusPopup.hpp"
#include "StatusMonitor.hpp"

using namespace geode::prelude;

class $modify(StatusMenuLayer, MenuLayer)
{
    bool init()
    {
        if (!MenuLayer::init())
            return false;

        // target menu as a fall back
        CCMenu *targetMenu = nullptr;
        if (auto right = this->getChildByID("right-side-menu"))
        {
            targetMenu = typeinfo_cast<CCMenu *>(right);
        }
        if (!targetMenu)
        {
            if (auto closeMenu = this->getChildByID("close-menu"))
            {
                targetMenu = typeinfo_cast<CCMenu *>(closeMenu);
            }
        }
        auto wifiIcon = CCSprite::create("wifiIcon.png"_spr);
        auto scene = CCDirector::sharedDirector()->getRunningScene();

        auto statusButton = CCMenuItemSpriteExtra::create(
            CircleButtonSprite::create(
                wifiIcon,
                CircleBaseColor::Green,
                CircleBaseSize::Medium),
            this,
            menu_selector(StatusMenuLayer::onStatusButton));

        statusButton->setScale(0.4f);

        if (targetMenu)
        {
            targetMenu->addChild(statusButton);
            targetMenu->updateLayout();
        }
        else
        {
            auto fallbackMenu = CCMenu::create();
            fallbackMenu->setID("internet-status-fallback-menu");
            fallbackMenu->setPosition({0.f, 0.f});
            this->addChild(fallbackMenu);

            auto winSize = CCDirector::sharedDirector()->getWinSize();
            statusButton->setPosition({winSize.width - 22.f, winSize.height - 22.f});
            fallbackMenu->addChild(statusButton);
        }

        // create StatusMonitor if it doesn't exist in the OverlayManager and keep it across scenes
        if (OverlayManager::get()->getChildByIDRecursive("status-monitor") == nullptr)
        {
            if (auto monitor = StatusMonitor::create())
            {
                monitor->setID("status-monitor");
                monitor->setPosition({0, 0});
                this->addChild(monitor);
                OverlayManager::get()->addChild(monitor);
            }
        }

        return true;
    }
    void onEnter()
    {
        MenuLayer::onEnter();
    }
    void onStatusButton(CCObject *)
    {
        if (auto popup = StatusPopup::create())
        {
            popup->show();
        }
    }
};

class $modify(CCLayer) {
    void onEnter() {
        CCLayer::onEnter();
        // log::debug("icon visibility check on level enter");
        geode::queueInMainThread([]() {
            auto overlay = OverlayManager::get();
            if (!overlay) return;
            for (auto node : overlay->getChildrenExt<StatusMonitor*>()) {
                if (auto monitor = typeinfo_cast<StatusMonitor*>(node)) {
                    monitor->applySettings();
                }
            }
        });
    }
};