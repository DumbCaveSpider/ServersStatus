#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class StatusMonitor : public CCMenu {
protected:
    bool init() override;
    void checkInternetStatus();
    void checkBoomlingsStatus();
    void checkGeodeStatus();
    void updateStatus(float);

    CCSprite* m_icon = nullptr;
public:
    void onEnter() override;
    static StatusMonitor* create();
};