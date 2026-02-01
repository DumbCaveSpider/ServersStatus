#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>

using namespace geode::prelude;
using namespace geode::utils;

class StatusPopup : public Popup {
     protected:
      bool init() override;
      void checkInternetStatus();
      void checkBoomlingsStatus();
      void checkGeodeStatus();
      void checkArgonStatus();
      void onModSettings(CCObject* sender);
      void onOpenCustomStatus(CCObject* sender);

      CCLabelBMFont* userInternet = nullptr;
      CCLabelBMFont* serverStatus = nullptr;
      CCLabelBMFont* geodeStatus = nullptr;
      CCLabelBMFont* argonStatus = nullptr;

      geode::async::TaskHolder<geode::utils::web::WebResponse> m_internetTask;
      geode::async::TaskHolder<geode::utils::web::WebResponse> m_boomlingsTask;
      geode::async::TaskHolder<geode::utils::web::WebResponse> m_geodeTask;
      geode::async::TaskHolder<geode::utils::web::WebResponse> m_argonTask;

     public:
      static StatusPopup* create();
      ~StatusPopup() {
          m_internetTask.cancel();
          m_boomlingsTask.cancel();
          m_geodeTask.cancel();
          m_argonTask.cancel();
      }
};