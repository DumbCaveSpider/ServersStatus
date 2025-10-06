#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

class StatusMonitor : public CCMenu
{
protected:
    bool init() override;
    void checkInternetStatus();
    void checkBoomlingsStatus();
    void checkGeodeStatus();
    void checkArgonStatus();

    CCSprite *m_icon = nullptr;

    bool m_internet_ok = false;
    bool m_boomlings_ok = false;
    bool m_geode_ok = false;
    bool m_argon_ok = false;

public:
    void onEnter() override;
    static StatusMonitor *create();
    void updateStatus(float);
};