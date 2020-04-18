// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/counters/site_settings_counter.h"

#include "base/test/simple_test_clock.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/browsing_data/core/browsing_data_utils.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(OS_ANDROID)
#include "content/public/browser/host_zoom_map.h"
#endif

namespace {

class SiteSettingsCounterTest : public testing::Test {
 public:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    map_ = HostContentSettingsMapFactory::GetForProfile(profile());
#if !defined(OS_ANDROID)
    zoom_map_ = content::HostZoomMap::GetDefaultForBrowserContext(profile());
#else
    zoom_map_ = nullptr;
#endif
    handler_registry_ = std::make_unique<ProtocolHandlerRegistry>(
        profile(), new ProtocolHandlerRegistry::Delegate());

    counter_ = std::make_unique<SiteSettingsCounter>(map(), zoom_map(),
                                                     handler_registry());
    counter_->Init(profile()->GetPrefs(),
                   browsing_data::ClearBrowsingDataTab::ADVANCED,
                   base::BindRepeating(&SiteSettingsCounterTest::Callback,
                                       base::Unretained(this)));
  }

  Profile* profile() { return profile_.get(); }

  HostContentSettingsMap* map() { return map_.get(); }

  content::HostZoomMap* zoom_map() { return zoom_map_; }

  ProtocolHandlerRegistry* handler_registry() {
    return handler_registry_.get();
  }

  SiteSettingsCounter* counter() { return counter_.get(); }

  void SetSiteSettingsDeletionPref(bool value) {
    profile()->GetPrefs()->SetBoolean(browsing_data::prefs::kDeleteSiteSettings,
                                      value);
  }

  void SetDeletionPeriodPref(browsing_data::TimePeriod period) {
    profile()->GetPrefs()->SetInteger(browsing_data::prefs::kDeleteTimePeriod,
                                      static_cast<int>(period));
  }

  browsing_data::BrowsingDataCounter::ResultInt GetResult() {
    DCHECK(finished_);
    return result_;
  }

  void Callback(
      std::unique_ptr<browsing_data::BrowsingDataCounter::Result> result) {
    DCHECK(result->Finished());
    finished_ = result->Finished();

    result_ = static_cast<browsing_data::BrowsingDataCounter::FinishedResult*>(
                  result.get())
                  ->Value();
  }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;

  scoped_refptr<HostContentSettingsMap> map_;
  content::HostZoomMap* zoom_map_;
  std::unique_ptr<ProtocolHandlerRegistry> handler_registry_;
  std::unique_ptr<SiteSettingsCounter> counter_;
  bool finished_;
  browsing_data::BrowsingDataCounter::ResultInt result_;
};

// Tests that the counter correctly counts each setting.
TEST_F(SiteSettingsCounterTest, Count) {
  map()->SetContentSettingDefaultScope(
      GURL("http://www.google.com"), GURL("http://www.google.com"),
      CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);
  map()->SetContentSettingDefaultScope(
      GURL("http://maps.google.com"), GURL("http://maps.google.com"),
      CONTENT_SETTINGS_TYPE_GEOLOCATION, std::string(), CONTENT_SETTING_ALLOW);

  counter()->Restart();
  EXPECT_EQ(2, GetResult());
}

// Test that the counter counts correctly when using a time period.
TEST_F(SiteSettingsCounterTest, CountWithTimePeriod) {
  base::SimpleTestClock test_clock;
  map()->SetClockForTesting(&test_clock);

  // Create a setting at Now()-90min.
  test_clock.SetNow(base::Time::Now() - base::TimeDelta::FromMinutes(90));
  map()->SetContentSettingDefaultScope(
      GURL("http://www.google.com"), GURL("http://www.google.com"),
      CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);

  // Create a setting at Now()-30min.
  test_clock.SetNow(base::Time::Now() - base::TimeDelta::FromMinutes(30));
  map()->SetContentSettingDefaultScope(
      GURL("http://maps.google.com"), GURL("http://maps.google.com"),
      CONTENT_SETTINGS_TYPE_GEOLOCATION, std::string(), CONTENT_SETTING_ALLOW);

  // Create a setting at Now()-31days.
  test_clock.SetNow(base::Time::Now() - base::TimeDelta::FromDays(31));
  map()->SetContentSettingDefaultScope(GURL("http://www.google.com"),
                                       GURL("http://www.google.com"),
                                       CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
                                       std::string(), CONTENT_SETTING_ALLOW);

  test_clock.SetNow(base::Time::Now());
  // Only one of the settings was created in the last hour.
  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_HOUR);
  EXPECT_EQ(1, GetResult());
  // Both settings were created during the last day.
  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_DAY);
  EXPECT_EQ(2, GetResult());
  // One of the settings was created 31days ago.
  SetDeletionPeriodPref(browsing_data::TimePeriod::OLDER_THAN_30_DAYS);
  EXPECT_EQ(1, GetResult());
}

// Tests that the counter doesn't count website settings
TEST_F(SiteSettingsCounterTest, OnlyCountContentSettings) {
  map()->SetContentSettingDefaultScope(
      GURL("http://www.google.com"), GURL("http://www.google.com"),
      CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);
  map()->SetWebsiteSettingDefaultScope(
      GURL("http://maps.google.com"), GURL(),
      CONTENT_SETTINGS_TYPE_SITE_ENGAGEMENT, std::string(),
      std::make_unique<base::DictionaryValue>());

  counter()->Restart();
  EXPECT_EQ(1, GetResult());
}

// Tests that the counter counts settings with the same pattern only
// once.
TEST_F(SiteSettingsCounterTest, OnlyCountPatternOnce) {
  map()->SetContentSettingDefaultScope(
      GURL("http://www.google.com"), GURL("http://www.google.com"),
      CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);
  map()->SetContentSettingDefaultScope(
      GURL("http://www.google.com"), GURL("http://www.google.com"),
      CONTENT_SETTINGS_TYPE_GEOLOCATION, std::string(), CONTENT_SETTING_ALLOW);

  counter()->Restart();
  EXPECT_EQ(1, GetResult());
}

// Tests that the counter starts counting automatically when the deletion
// pref changes to true.
TEST_F(SiteSettingsCounterTest, PrefChanged) {
  SetSiteSettingsDeletionPref(false);
  map()->SetContentSettingDefaultScope(
      GURL("http://www.google.com"), GURL("http://www.google.com"),
      CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);

  SetSiteSettingsDeletionPref(true);
  EXPECT_EQ(1, GetResult());
}

// Tests that changing the deletion period restarts the counting.
TEST_F(SiteSettingsCounterTest, PeriodChanged) {
  map()->SetContentSettingDefaultScope(
      GURL("http://www.google.com"), GURL("http://www.google.com"),
      CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);

  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_HOUR);
  EXPECT_EQ(1, GetResult());
}

#if !defined(OS_ANDROID)
TEST_F(SiteSettingsCounterTest, ZoomLevel) {
  zoom_map()->SetZoomLevelForHost("google.com", 1.5);
  zoom_map()->SetZoomLevelForHost("www.google.com", 1.5);

  counter()->Restart();
  EXPECT_EQ(2, GetResult());
}

TEST_F(SiteSettingsCounterTest, ZoomAndContentSettingAndHandlers) {
  zoom_map()->SetZoomLevelForHost("google.com", 1.5);
  zoom_map()->SetZoomLevelForHost("www.google.com", 1.5);
  map()->SetContentSettingDefaultScope(
      GURL("https://www.google.com"), GURL("https://www.google.com"),
      CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);
  map()->SetContentSettingDefaultScope(
      GURL("https://maps.google.com"), GURL("https://maps.google.com"),
      CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);
  base::Time now = base::Time::Now();
  handler_registry()->OnAcceptRegisterProtocolHandler(
      ProtocolHandler("test1", GURL("http://www.google.com"), now));
  handler_registry()->OnAcceptRegisterProtocolHandler(
      ProtocolHandler("test1", GURL("http://docs.google.com"), now));

  counter()->Restart();
  EXPECT_EQ(4, GetResult());
}
#endif

TEST_F(SiteSettingsCounterTest, ProtocolHandlerCounting) {
  base::Time now = base::Time::Now();

  handler_registry()->OnAcceptRegisterProtocolHandler(
      ProtocolHandler("test1", GURL("http://www.google.com"), now));
  handler_registry()->OnAcceptRegisterProtocolHandler(
      ProtocolHandler("test2", GURL("http://maps.google.com"),
                      now - base::TimeDelta::FromMinutes(90)));
  EXPECT_TRUE(handler_registry()->IsHandledProtocol("test1"));
  EXPECT_TRUE(handler_registry()->IsHandledProtocol("test2"));

  SetDeletionPeriodPref(browsing_data::TimePeriod::ALL_TIME);
  EXPECT_EQ(2, GetResult());
  SetDeletionPeriodPref(browsing_data::TimePeriod::LAST_HOUR);
  EXPECT_EQ(1, GetResult());
}

}  // namespace
