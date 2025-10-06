#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class StatusPopup : public Popup<>
{
protected:
    bool setup() override;
    void checkInternetStatus();
    void checkBoomlingsStatus();
    void checkGeodeStatus();
    void checkArgonStatus();
    void onModSettings(CCObject *sender);

    CCLabelBMFont *userInternet = nullptr;
    CCLabelBMFont *serverStatus = nullptr;
    CCLabelBMFont *geodeStatus = nullptr;
    CCLabelBMFont *argonStatus = nullptr;

public:
    static StatusPopup *create();
};