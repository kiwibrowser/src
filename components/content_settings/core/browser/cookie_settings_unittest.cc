// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/content_settings/core/browser/cookie_settings.h"

#include "base/message_loop/message_loop.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content_settings {

namespace {

class CookieSettingsTest : public testing::Test {
 public:
  CookieSettingsTest()
      : kBlockedSite("http://ads.thirdparty.com"),
        kAllowedSite("http://good.allays.com"),
        kFirstPartySite("http://cool.things.com"),
        kChromeURL("chrome://foo"),
        kExtensionURL("chrome-extension://deadbeef"),
        kDomain("example.com"),
        kDotDomain(".example.com"),
        kSubDomain("www.example.com"),
        kHttpSite("http://example.com"),
        kHttpsSite("https://example.com"),
        kHttpsSubdomainSite("https://www.example.com"),
        kHttpsSite8080("https://example.com:8080"),
        kAllHttpsSitesPattern(ContentSettingsPattern::FromString("https://*")) {
    CookieSettings::RegisterProfilePrefs(prefs_.registry());
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    settings_map_ = new HostContentSettingsMap(
        &prefs_, false /* incognito_profile */, false /* guest_profile */,
        false /* store_last_modified */);
    cookie_settings_ =
        new CookieSettings(settings_map_.get(), &prefs_, "chrome-extension");
  }

  ~CookieSettingsTest() override { settings_map_->ShutdownOnUIThread(); }

 protected:
  bool ShouldDeleteCookieOnExit(const std::string& domain, bool is_https) {
    ContentSettingsForOneType settings;
    cookie_settings_->GetCookieSettings(&settings);
    return cookie_settings_->ShouldDeleteCookieOnExit(settings, domain,
                                                      is_https);
  }

  // There must be a valid ThreadTaskRunnerHandle in HostContentSettingsMap's
  // scope.
  base::MessageLoop message_loop_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> settings_map_;
  scoped_refptr<CookieSettings> cookie_settings_;
  const GURL kBlockedSite;
  const GURL kAllowedSite;
  const GURL kFirstPartySite;
  const GURL kChromeURL;
  const GURL kExtensionURL;
  const std::string kDomain;
  const std::string kDotDomain;
  const std::string kSubDomain;
  const GURL kHttpSite;
  const GURL kHttpsSite;
  const GURL kHttpsSubdomainSite;
  const GURL kHttpsSite8080;
  ContentSettingsPattern kAllHttpsSitesPattern;
};

TEST_F(CookieSettingsTest, TestWhitelistedScheme) {
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_BLOCK);
  EXPECT_FALSE(cookie_settings_->IsCookieAccessAllowed(kHttpSite, kChromeURL));
  EXPECT_TRUE(cookie_settings_->IsCookieAccessAllowed(kHttpsSite, kChromeURL));
  EXPECT_TRUE(cookie_settings_->IsCookieAccessAllowed(kChromeURL, kHttpSite));
#if BUILDFLAG(ENABLE_EXTENSIONS)
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kExtensionURL, kExtensionURL));
#else
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kExtensionURL, kExtensionURL));
#endif
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kExtensionURL, kHttpSite));
}

TEST_F(CookieSettingsTest, CookiesBlockSingle) {
  cookie_settings_->SetCookieSetting(kBlockedSite, CONTENT_SETTING_BLOCK);
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kBlockedSite, kBlockedSite));
}

TEST_F(CookieSettingsTest, CookiesBlockThirdParty) {
  prefs_.SetBoolean(prefs::kBlockThirdPartyCookies, true);
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kBlockedSite, kFirstPartySite));
  EXPECT_FALSE(cookie_settings_->IsCookieSessionOnly(kBlockedSite));
}

TEST_F(CookieSettingsTest, CookiesAllowThirdParty) {
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kBlockedSite, kFirstPartySite));
  EXPECT_FALSE(cookie_settings_->IsCookieSessionOnly(kBlockedSite));
}

TEST_F(CookieSettingsTest, CookiesExplicitBlockSingleThirdParty) {
  cookie_settings_->SetCookieSetting(kBlockedSite, CONTENT_SETTING_BLOCK);
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kBlockedSite, kFirstPartySite));
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kAllowedSite, kFirstPartySite));
}

TEST_F(CookieSettingsTest, CookiesExplicitSessionOnly) {
  cookie_settings_->SetCookieSetting(kBlockedSite,
                                     CONTENT_SETTING_SESSION_ONLY);
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kBlockedSite, kFirstPartySite));
  EXPECT_TRUE(cookie_settings_->IsCookieSessionOnly(kBlockedSite));

  prefs_.SetBoolean(prefs::kBlockThirdPartyCookies, true);
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kBlockedSite, kFirstPartySite));
  EXPECT_TRUE(cookie_settings_->IsCookieSessionOnly(kBlockedSite));
}

TEST_F(CookieSettingsTest, KeepBlocked) {
  // Keep blocked cookies.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_ALLOW);
  cookie_settings_->SetCookieSetting(kHttpsSite, CONTENT_SETTING_BLOCK);
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, true));
}

TEST_F(CookieSettingsTest, DeleteSessionOnly) {
  // Keep session_only http cookies if https is allowed.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_SESSION_ONLY);
  cookie_settings_->SetCookieSetting(kHttpsSite, CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kSubDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kSubDomain, true));

  // Delete cookies if site is session only.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_BLOCK);
  cookie_settings_->SetCookieSetting(kHttpsSite, CONTENT_SETTING_SESSION_ONLY);
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDotDomain, true));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kSubDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kSubDomain, true));

  // Http blocked, https allowed - keep secure and non secure cookies.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_SESSION_ONLY);
  cookie_settings_->SetCookieSetting(kHttpSite, CONTENT_SETTING_BLOCK);
  cookie_settings_->SetCookieSetting(kHttpsSite, CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kSubDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kSubDomain, true));

  // Http and https session only, all is deleted.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_ALLOW);
  cookie_settings_->SetCookieSetting(kHttpSite, CONTENT_SETTING_SESSION_ONLY);
  cookie_settings_->SetCookieSetting(kHttpsSite, CONTENT_SETTING_SESSION_ONLY);
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDotDomain, true));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kSubDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kSubDomain, true));
}

TEST_F(CookieSettingsTest, DeletionWithDifferentPorts) {
  // Keep cookies for site with special port.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_SESSION_ONLY);
  cookie_settings_->SetCookieSetting(kHttpsSite8080, CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, true));

  // Delete cookies with special port.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_BLOCK);
  cookie_settings_->SetCookieSetting(kHttpsSite8080,
                                     CONTENT_SETTING_SESSION_ONLY);
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDotDomain, true));
}

TEST_F(CookieSettingsTest, DeletionWithSubDomains) {
  // Cookies accessible by subdomains are kept.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_SESSION_ONLY);
  cookie_settings_->SetCookieSetting(kHttpsSubdomainSite,
                                     CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, true));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kSubDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kSubDomain, true));

  // Cookies that have a session_only subdomain but are accessible by allowed
  // domains are kept.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_ALLOW);
  cookie_settings_->SetCookieSetting(kHttpsSubdomainSite,
                                     CONTENT_SETTING_SESSION_ONLY);
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDotDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kSubDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kSubDomain, true));

  // Cookies created by session_only subdomains are deleted.
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_BLOCK);
  cookie_settings_->SetCookieSetting(kHttpsSubdomainSite,
                                     CONTENT_SETTING_SESSION_ONLY);
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDotDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kDotDomain, true));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, false));
  EXPECT_FALSE(ShouldDeleteCookieOnExit(kDomain, true));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kSubDomain, false));
  EXPECT_TRUE(ShouldDeleteCookieOnExit(kSubDomain, true));
}

TEST_F(CookieSettingsTest, CookiesThirdPartyBlockedExplicitAllow) {
  cookie_settings_->SetCookieSetting(kAllowedSite, CONTENT_SETTING_ALLOW);
  prefs_.SetBoolean(prefs::kBlockThirdPartyCookies, true);
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kAllowedSite, kFirstPartySite));
  EXPECT_FALSE(cookie_settings_->IsCookieSessionOnly(kAllowedSite));

  // Extensions should always be allowed to use cookies.
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kAllowedSite, kExtensionURL));
}

TEST_F(CookieSettingsTest, CookiesThirdPartyBlockedAllSitesAllowed) {
  cookie_settings_->SetCookieSetting(kAllowedSite, CONTENT_SETTING_ALLOW);
  prefs_.SetBoolean(prefs::kBlockThirdPartyCookies, true);
  // As an example for a url that matches all hosts but not all origins,
  // match all HTTPS sites.
  settings_map_->SetContentSettingCustomScope(
      kAllHttpsSitesPattern, ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_COOKIES, std::string(), CONTENT_SETTING_ALLOW);
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_SESSION_ONLY);

  // |kAllowedSite| should be allowed.
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kAllowedSite, kBlockedSite));
  EXPECT_FALSE(cookie_settings_->IsCookieSessionOnly(kAllowedSite));

  // HTTPS sites should be allowed in a first-party context.
  EXPECT_TRUE(cookie_settings_->IsCookieAccessAllowed(kHttpsSite, kHttpsSite));
  EXPECT_FALSE(cookie_settings_->IsCookieSessionOnly(kAllowedSite));

  // HTTP sites should be allowed, but session-only.
  EXPECT_TRUE(cookie_settings_->IsCookieAccessAllowed(kFirstPartySite,
                                                      kFirstPartySite));
  EXPECT_TRUE(cookie_settings_->IsCookieSessionOnly(kFirstPartySite));

  // Third-party cookies should be blocked.
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kFirstPartySite, kBlockedSite));
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kHttpsSite, kBlockedSite));
}

TEST_F(CookieSettingsTest, CookiesBlockEverything) {
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_BLOCK);

  EXPECT_FALSE(cookie_settings_->IsCookieAccessAllowed(kFirstPartySite,
                                                       kFirstPartySite));
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kAllowedSite, kFirstPartySite));
}

TEST_F(CookieSettingsTest, CookiesBlockEverythingExceptAllowed) {
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_BLOCK);
  cookie_settings_->SetCookieSetting(kAllowedSite, CONTENT_SETTING_ALLOW);
  EXPECT_FALSE(cookie_settings_->IsCookieAccessAllowed(kFirstPartySite,
                                                       kFirstPartySite));
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kAllowedSite, kFirstPartySite));
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kAllowedSite, kAllowedSite));
  EXPECT_FALSE(cookie_settings_->IsCookieSessionOnly(kAllowedSite));
}

TEST_F(CookieSettingsTest, ExtensionsRegularSettings) {
  cookie_settings_->SetCookieSetting(kBlockedSite, CONTENT_SETTING_BLOCK);

  // Regular cookie settings also apply to extensions.
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kBlockedSite, kExtensionURL));
}

TEST_F(CookieSettingsTest, ExtensionsOwnCookies) {
  cookie_settings_->SetDefaultCookieSetting(CONTENT_SETTING_BLOCK);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Extensions can always use cookies (and site data) in their own origin.
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kExtensionURL, kExtensionURL));
#else
  // Except if extensions are disabled. Then the extension-specific checks do
  // not exist and the default setting is to block.
  EXPECT_FALSE(
      cookie_settings_->IsCookieAccessAllowed(kExtensionURL, kExtensionURL));
#endif
}

TEST_F(CookieSettingsTest, ExtensionsThirdParty) {
  prefs_.SetBoolean(prefs::kBlockThirdPartyCookies, true);

  // XHRs stemming from extensions are exempt from third-party cookie blocking
  // rules (as the first party is always the extension's security origin).
  EXPECT_TRUE(
      cookie_settings_->IsCookieAccessAllowed(kBlockedSite, kExtensionURL));
}

}  // namespace

}  // namespace content_settings
