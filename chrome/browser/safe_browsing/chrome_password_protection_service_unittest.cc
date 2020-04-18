// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/safe_browsing/chrome_password_protection_service.h"

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/extensions/api/safe_browsing_private/safe_browsing_private_event_router_factory.h"
#include "chrome/browser/safe_browsing/test_extension_event_observer.h"
#include "chrome/browser/safe_browsing/ui_manager.h"
#include "chrome/browser/signin/account_fetcher_service_factory.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/fake_account_fetcher_service_builder.h"
#include "chrome/browser/signin/fake_profile_oauth2_token_service_builder.h"
#include "chrome/browser/signin/fake_signin_manager_builder.h"
#include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/test_signin_client_builder.h"
#include "chrome/browser/sync/user_event_service_factory.h"
#include "chrome/common/extensions/api/safe_browsing_private.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "components/safe_browsing/features.h"
#include "components/safe_browsing/password_protection/password_protection_navigation_throttle.h"
#include "components/safe_browsing/password_protection/password_protection_request.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/fake_account_fetcher_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "components/sync/user_events/fake_user_event_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/variations/variations_params_manager.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/test_event_router.h"
#include "net/http/http_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using sync_pb::UserEventSpecifics;
using GaiaPasswordReuse = UserEventSpecifics::GaiaPasswordReuse;
using PasswordReuseDialogInteraction =
    GaiaPasswordReuse::PasswordReuseDialogInteraction;
using PasswordReuseLookup = GaiaPasswordReuse::PasswordReuseLookup;

namespace OnPolicySpecifiedPasswordReuseDetected = extensions::api::
    safe_browsing_private::OnPolicySpecifiedPasswordReuseDetected;
namespace OnPolicySpecifiedPasswordChanged =
    extensions::api::safe_browsing_private::OnPolicySpecifiedPasswordChanged;

namespace safe_browsing {

namespace {

const char kPhishingURL[] = "http://phishing.com/";
const char kPasswordReuseURL[] = "http://login.example.com/";
const char kTestAccountID[] = "account_id";
const char kTestEmail[] = "foo@example.com";
const char kBasicResponseHeaders[] = "HTTP/1.1 200 OK";
const char kRedirectURL[] = "http://redirect.com";

std::unique_ptr<KeyedService> BuildFakeUserEventService(
    content::BrowserContext* context) {
  return std::make_unique<syncer::FakeUserEventService>();
}

constexpr struct {
  // The response from the password protection service.
  PasswordProtectionService::RequestOutcome request_outcome;
  // The enum to log in the user event for that response.
  PasswordReuseLookup::LookupResult lookup_result;
} kTestCasesWithoutVerdict[]{
    {PasswordProtectionService::MATCHED_WHITELIST,
     PasswordReuseLookup::WHITELIST_HIT},
    {PasswordProtectionService::URL_NOT_VALID_FOR_REPUTATION_COMPUTING,
     PasswordReuseLookup::URL_UNSUPPORTED},
    {PasswordProtectionService::CANCELED, PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::TIMEDOUT, PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::DISABLED_DUE_TO_INCOGNITO,
     PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::REQUEST_MALFORMED,
     PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::FETCH_FAILED,
     PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::RESPONSE_MALFORMED,
     PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::SERVICE_DESTROYED,
     PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::DISABLED_DUE_TO_FEATURE_DISABLED,
     PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::DISABLED_DUE_TO_USER_POPULATION,
     PasswordReuseLookup::REQUEST_FAILURE},
    {PasswordProtectionService::MAX_OUTCOME,
     PasswordReuseLookup::REQUEST_FAILURE}};

}  // namespace

class MockChromePasswordProtectionService
    : public ChromePasswordProtectionService {
 public:
  explicit MockChromePasswordProtectionService(
      Profile* profile,
      scoped_refptr<HostContentSettingsMap> content_setting_map,
      scoped_refptr<SafeBrowsingUIManager> ui_manager)
      : ChromePasswordProtectionService(profile,
                                        content_setting_map,
                                        ui_manager),
        is_incognito_(false),
        is_extended_reporting_(false) {}
  bool IsExtendedReporting() override { return is_extended_reporting_; }
  bool IsIncognito() override { return is_incognito_; }

  // Configures the results returned by IsExtendedReporting(), IsIncognito(),
  // and IsHistorySyncEnabled().
  void ConfigService(bool is_incognito, bool is_extended_reporting) {
    is_incognito_ = is_incognito;
    is_extended_reporting_ = is_extended_reporting;
  }

  SafeBrowsingUIManager* ui_manager() { return ui_manager_.get(); }

 protected:
  friend class ChromePasswordProtectionServiceTest;

 private:
  bool is_incognito_;
  bool is_extended_reporting_;
};

class ChromePasswordProtectionServiceTest
    : public ChromeRenderViewHostTestHarness {
 public:
  ChromePasswordProtectionServiceTest() {}

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    profile()->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, true);
    HostContentSettingsMap::RegisterProfilePrefs(test_pref_service_.registry());
    content_setting_map_ = new HostContentSettingsMap(
        &test_pref_service_, false /* incognito */, false /* guest_profile */,
        false /* store_last_modified */);
    service_ = std::make_unique<MockChromePasswordProtectionService>(
        profile(), content_setting_map_,
        new SafeBrowsingUIManager(
            SafeBrowsingService::CreateSafeBrowsingService()));
    fake_user_event_service_ = static_cast<syncer::FakeUserEventService*>(
        browser_sync::UserEventServiceFactory::GetInstance()
            ->SetTestingFactoryAndUse(browser_context(),
                                      &BuildFakeUserEventService));
    test_event_router_ =
        extensions::CreateAndUseTestEventRouter(browser_context());
    extensions::SafeBrowsingPrivateEventRouterFactory::GetInstance()
        ->SetTestingFactory(browser_context(),
                            BuildSafeBrowsingPrivateEventRouter);
  }

  void TearDown() override {
    base::RunLoop().RunUntilIdle();
    service_.reset();
    request_ = nullptr;
    content_setting_map_->ShutdownOnUIThread();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  content::BrowserContext* CreateBrowserContext() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(ProfileOAuth2TokenServiceFactory::GetInstance(),
                              BuildFakeProfileOAuth2TokenService);
    builder.AddTestingFactory(ChromeSigninClientFactory::GetInstance(),
                              signin::BuildTestSigninClient);
    builder.AddTestingFactory(SigninManagerFactory::GetInstance(),
                              BuildFakeSigninManagerBase);
    builder.AddTestingFactory(AccountFetcherServiceFactory::GetInstance(),
                              FakeAccountFetcherServiceBuilder::BuildForTests);
    return builder.Build().release();
  }

  syncer::FakeUserEventService* GetUserEventService() {
    return fake_user_event_service_;
  }

  void InitializeRequest(LoginReputationClientRequest::TriggerType type) {
    if (type == LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE) {
      request_ = new PasswordProtectionRequest(
          web_contents(), GURL(kPhishingURL), GURL(), GURL(), false,
          std::vector<std::string>({"somedomain.com"}), type, true,
          service_.get(), 0);
    } else {
      ASSERT_EQ(LoginReputationClientRequest::PASSWORD_REUSE_EVENT, type);
      request_ = new PasswordProtectionRequest(
          web_contents(), GURL(kPhishingURL), GURL(), GURL(), true,
          std::vector<std::string>(), type, true, service_.get(), 0);
    }
  }

  void InitializeVerdict(LoginReputationClientResponse::VerdictType type) {
    verdict_ = std::make_unique<LoginReputationClientResponse>();
    verdict_->set_verdict_type(type);
  }

  void SimulateRequestFinished(
      LoginReputationClientResponse::VerdictType verdict_type) {
    std::unique_ptr<LoginReputationClientResponse> verdict =
        std::make_unique<LoginReputationClientResponse>();
    verdict->set_verdict_type(verdict_type);
    service_->RequestFinished(request_.get(), false, std::move(verdict));
  }

  void SetUpSyncAccount(const std::string& hosted_domain,
                        const std::string& account_id,
                        const std::string& email) {
    FakeAccountFetcherService* account_fetcher_service =
        static_cast<FakeAccountFetcherService*>(
            AccountFetcherServiceFactory::GetForProfile(profile()));
    AccountTrackerService* account_tracker_service =
        AccountTrackerServiceFactory::GetForProfile(profile());
    account_fetcher_service->FakeUserInfoFetchSuccess(
        account_tracker_service->PickAccountIdForAccount(account_id, email),
        email, account_id, hosted_domain, "full_name", "given_name", "locale",
        "http://picture.example.com/picture.jpg");
  }

  void PrepareRequest(LoginReputationClientRequest::TriggerType trigger_type,
                      bool is_warning_showing) {
    InitializeRequest(trigger_type);
    request_->set_is_modal_warning_showing(is_warning_showing);
    service_->pending_requests_.insert(request_);
  }

  content::NavigationThrottle::ThrottleCheckResult SimulateWillStart(
      content::NavigationHandle* test_handle) {
    std::unique_ptr<PasswordProtectionNavigationThrottle> throttle =
        service_->MaybeCreateNavigationThrottle(test_handle);
    if (throttle)
      test_handle->RegisterThrottleForTesting(std::move(throttle));

    return test_handle->CallWillStartRequestForTesting();
  }

  int GetSizeofUnhandledSyncPasswordReuses() {
    DictionaryPrefUpdate unhandled_sync_password_reuses(
        profile()->GetPrefs(), prefs::kSafeBrowsingUnhandledSyncPasswordReuses);
    return unhandled_sync_password_reuses->size();
  }

  size_t GetNumberOfNavigationThrottles() {
    return request_ ? request_->throttles_.size() : 0u;
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable test_pref_service_;
  scoped_refptr<HostContentSettingsMap> content_setting_map_;
  std::unique_ptr<MockChromePasswordProtectionService> service_;
  scoped_refptr<PasswordProtectionRequest> request_;
  std::unique_ptr<LoginReputationClientResponse> verdict_;
  // Owned by KeyedServiceFactory.
  syncer::FakeUserEventService* fake_user_event_service_;
  extensions::TestEventRouter* test_event_router_;
};

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyUserPopulationForPasswordOnFocusPing) {
  // Password field on focus pinging is enabled on !incognito && SBER.
  PasswordProtectionService::RequestOutcome reason;
  service_->ConfigService(false /*incognito*/, false /*SBER*/);
  EXPECT_FALSE(service_->IsPingingEnabled(
      LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE, &reason));
  EXPECT_EQ(PasswordProtectionService::DISABLED_DUE_TO_USER_POPULATION, reason);

  service_->ConfigService(false /*incognito*/, true /*SBER*/);
  EXPECT_TRUE(service_->IsPingingEnabled(
      LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE, &reason));

  service_->ConfigService(true /*incognito*/, false /*SBER*/);
  EXPECT_FALSE(service_->IsPingingEnabled(
      LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE, &reason));
  EXPECT_EQ(PasswordProtectionService::DISABLED_DUE_TO_INCOGNITO, reason);

  service_->ConfigService(true /*incognito*/, true /*SBER*/);
  EXPECT_FALSE(service_->IsPingingEnabled(
      LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE, &reason));
  EXPECT_EQ(PasswordProtectionService::DISABLED_DUE_TO_INCOGNITO, reason);
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyUserPopulationForProtectedPasswordEntryPing) {
  SigninManagerFactory::GetForProfile(profile())->SetAuthenticatedAccountInfo(
      kTestAccountID, kTestEmail);
  SetUpSyncAccount(std::string(AccountTrackerService::kNoHostedDomainFound),
                   std::string(kTestAccountID), std::string(kTestEmail));

  // Protected password entry pinging is enabled by default.
  PasswordProtectionService::RequestOutcome reason;
  service_->ConfigService(false /*incognito*/, false /*SBER*/);
  EXPECT_TRUE(service_->IsPingingEnabled(
      LoginReputationClientRequest::PASSWORD_REUSE_EVENT, &reason));

  service_->ConfigService(false /*incognito*/, true /*SBER*/);
  EXPECT_TRUE(service_->IsPingingEnabled(
      LoginReputationClientRequest::PASSWORD_REUSE_EVENT, &reason));

  service_->ConfigService(true /*incognito*/, false /*SBER*/);
  EXPECT_TRUE(service_->IsPingingEnabled(
      LoginReputationClientRequest::PASSWORD_REUSE_EVENT, &reason));

  service_->ConfigService(true /*incognito*/, true /*SBER*/);
  EXPECT_TRUE(service_->IsPingingEnabled(
      LoginReputationClientRequest::PASSWORD_REUSE_EVENT, &reason));

  // If password protection pinging is disabled by policy,
  // |IsPingingEnabled(..)| should return false.
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      safe_browsing::kEnterprisePasswordProtectionV1);
  profile()->GetPrefs()->SetInteger(prefs::kPasswordProtectionWarningTrigger,
                                    PASSWORD_PROTECTION_OFF);
  service_->ConfigService(false /*incognito*/, false /*SBER*/);
  EXPECT_FALSE(service_->IsPingingEnabled(
      LoginReputationClientRequest::PASSWORD_REUSE_EVENT, &reason));
  EXPECT_EQ(PasswordProtectionService::TURNED_OFF_BY_ADMIN, reason);

  profile()->GetPrefs()->SetInteger(prefs::kPasswordProtectionWarningTrigger,
                                    PASSWORD_REUSE);
  EXPECT_FALSE(service_->IsPingingEnabled(
      LoginReputationClientRequest::PASSWORD_REUSE_EVENT, &reason));
  EXPECT_EQ(PasswordProtectionService::PASSWORD_ALERT_MODE, reason);
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyPingingIsSkippedIfMatchEnterpriseWhitelist) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      safe_browsing::kEnterprisePasswordProtectionV1);
  PasswordProtectionService::RequestOutcome reason =
      PasswordProtectionService::UNKNOWN;
  ASSERT_FALSE(
      profile()->GetPrefs()->HasPrefPath(prefs::kSafeBrowsingWhitelistDomains));

  // If there's no whitelist, IsURLWhitelistedForPasswordEntry(_,_) should
  // return false.
  EXPECT_FALSE(service_->IsURLWhitelistedForPasswordEntry(
      GURL("https://www.mydomain.com"), &reason));
  EXPECT_EQ(PasswordProtectionService::UNKNOWN, reason);

  // Verify if match enterprise whitelist.
  base::ListValue whitelist;
  whitelist.AppendString("mydomain.com");
  whitelist.AppendString("mydomain.net");
  profile()->GetPrefs()->Set(prefs::kSafeBrowsingWhitelistDomains, whitelist);
  EXPECT_TRUE(service_->IsURLWhitelistedForPasswordEntry(
      GURL("https://www.mydomain.com"), &reason));
  EXPECT_EQ(PasswordProtectionService::MATCHED_ENTERPRISE_WHITELIST, reason);

  // Verify if matches enterprise change password url.
  profile()->GetPrefs()->ClearPref(prefs::kSafeBrowsingWhitelistDomains);
  reason = PasswordProtectionService::UNKNOWN;
  EXPECT_FALSE(service_->IsURLWhitelistedForPasswordEntry(
      GURL("https://www.mydomain.com"), &reason));
  EXPECT_EQ(PasswordProtectionService::UNKNOWN, reason);

  profile()->GetPrefs()->SetString(prefs::kPasswordProtectionChangePasswordURL,
                                   "https://mydomain.com/change_password.html");
  EXPECT_TRUE(service_->IsURLWhitelistedForPasswordEntry(
      GURL("https://mydomain.com/change_password.html#ref?user_name=alice"),
      &reason));
  EXPECT_EQ(PasswordProtectionService::MATCHED_ENTERPRISE_CHANGE_PASSWORD_URL,
            reason);

  // Verify if matches enterprise login url.
  profile()->GetPrefs()->ClearPref(prefs::kSafeBrowsingWhitelistDomains);
  profile()->GetPrefs()->ClearPref(prefs::kPasswordProtectionChangePasswordURL);
  reason = PasswordProtectionService::UNKNOWN;
  EXPECT_FALSE(service_->IsURLWhitelistedForPasswordEntry(
      GURL("https://www.mydomain.com"), &reason));
  EXPECT_EQ(PasswordProtectionService::UNKNOWN, reason);
  base::ListValue login_urls;
  login_urls.AppendString("https://mydomain.com/login.html");
  profile()->GetPrefs()->Set(prefs::kPasswordProtectionLoginURLs, login_urls);
  EXPECT_TRUE(service_->IsURLWhitelistedForPasswordEntry(
      GURL("https://mydomain.com/login.html#ref?user_name=alice"), &reason));
  EXPECT_EQ(PasswordProtectionService::MATCHED_ENTERPRISE_LOGIN_URL, reason);
}

TEST_F(ChromePasswordProtectionServiceTest, VerifyGetSyncAccountType) {
  EXPECT_EQ(LoginReputationClientRequest::PasswordReuseEvent::NOT_SIGNED_IN,
            service_->GetSyncAccountType());
  EXPECT_TRUE(service_->GetOrganizationName().empty());
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->SetAuthenticatedAccountInfo(kTestAccountID, kTestEmail);
  SetUpSyncAccount(std::string(AccountTrackerService::kNoHostedDomainFound),
                   std::string(kTestAccountID),
                   std::string(kTestEmail /*foo@example.com*/));
  EXPECT_EQ(LoginReputationClientRequest::PasswordReuseEvent::GMAIL,
            service_->GetSyncAccountType());
  EXPECT_EQ("example.com", service_->GetOrganizationName());

  SetUpSyncAccount("example.edu", std::string(kTestAccountID),
                   std::string(kTestEmail /*foo@example.com*/));
  EXPECT_EQ(LoginReputationClientRequest::PasswordReuseEvent::GSUITE,
            service_->GetSyncAccountType());
  EXPECT_EQ("example.com", service_->GetOrganizationName());
}

TEST_F(ChromePasswordProtectionServiceTest, VerifyUpdateSecurityState) {
  GURL url("http://password_reuse_url.com");
  NavigateAndCommit(url);
  SBThreatType current_threat_type = SB_THREAT_TYPE_UNUSED;
  ASSERT_FALSE(service_->ui_manager()->IsUrlWhitelistedOrPendingForWebContents(
      url, false, web_contents()->GetController().GetLastCommittedEntry(),
      web_contents(), false, &current_threat_type));
  EXPECT_EQ(SB_THREAT_TYPE_UNUSED, current_threat_type);

  // Cache a verdict for this URL.
  LoginReputationClientResponse verdict_proto;
  verdict_proto.set_verdict_type(LoginReputationClientResponse::PHISHING);
  verdict_proto.set_cache_duration_sec(600);
  verdict_proto.set_cache_expression("password_reuse_url.com/");
  service_->CacheVerdict(url,
                         LoginReputationClientRequest::PASSWORD_REUSE_EVENT,
                         &verdict_proto, base::Time::Now());

  service_->UpdateSecurityState(SB_THREAT_TYPE_PASSWORD_REUSE, web_contents());
  ASSERT_TRUE(service_->ui_manager()->IsUrlWhitelistedOrPendingForWebContents(
      url, false, web_contents()->GetController().GetLastCommittedEntry(),
      web_contents(), false, &current_threat_type));
  EXPECT_EQ(SB_THREAT_TYPE_PASSWORD_REUSE, current_threat_type);

  service_->UpdateSecurityState(safe_browsing::SB_THREAT_TYPE_SAFE,
                                web_contents());
  current_threat_type = SB_THREAT_TYPE_UNUSED;
  service_->ui_manager()->IsUrlWhitelistedOrPendingForWebContents(
      url, false, web_contents()->GetController().GetLastCommittedEntry(),
      web_contents(), false, &current_threat_type);
  EXPECT_EQ(SB_THREAT_TYPE_UNUSED, current_threat_type);
  LoginReputationClientResponse verdict;
  EXPECT_EQ(
      LoginReputationClientResponse::SAFE,
      service_->GetCachedVerdict(
          url, LoginReputationClientRequest::PASSWORD_REUSE_EVENT, &verdict));
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyPasswordReuseUserEventNotRecorded) {
  // Feature not enabled so nothing should be logged.
  NavigateAndCommit(GURL("https:www.example.com/"));

  // PasswordReuseDetected
  service_->MaybeLogPasswordReuseDetectedEvent(web_contents());
  EXPECT_TRUE(GetUserEventService()->GetRecordedUserEvents().empty());
  service_->MaybeLogPasswordReuseLookupEvent(
      web_contents(), PasswordProtectionService::MATCHED_WHITELIST, nullptr);
  EXPECT_TRUE(GetUserEventService()->GetRecordedUserEvents().empty());

  // PasswordReuseLookup
  unsigned long t = 0;
  for (const auto& it : kTestCasesWithoutVerdict) {
    service_->MaybeLogPasswordReuseLookupEvent(web_contents(),
                                               it.request_outcome, nullptr);
    ASSERT_TRUE(GetUserEventService()->GetRecordedUserEvents().empty()) << t;
    t++;
  }

  // PasswordReuseDialogInteraction
  service_->LogPasswordReuseDialogInteraction(
      1000 /* navigation_id */,
      PasswordReuseDialogInteraction::WARNING_ACTION_TAKEN);
  ASSERT_TRUE(GetUserEventService()->GetRecordedUserEvents().empty());
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyPasswordReuseDetectedUserEventRecorded) {
  // Configure sync account type to GMAIL.
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->SetAuthenticatedAccountInfo(kTestAccountID, kTestEmail);
  SetUpSyncAccount(std::string(AccountTrackerService::kNoHostedDomainFound),
                   std::string(kTestAccountID), std::string(kTestEmail));
  EXPECT_EQ(LoginReputationClientRequest::PasswordReuseEvent::GMAIL,
            service_->GetSyncAccountType());

  NavigateAndCommit(GURL("https://www.example.com/"));

  // Case 1: safe_browsing_enabled = true
  profile()->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, true);
  service_->MaybeLogPasswordReuseDetectedEvent(web_contents());
  ASSERT_EQ(1ul, GetUserEventService()->GetRecordedUserEvents().size());
  GaiaPasswordReuse event = GetUserEventService()
                                ->GetRecordedUserEvents()[0]
                                .gaia_password_reuse_event();
  EXPECT_TRUE(event.reuse_detected().status().enabled());

  // Case 2: safe_browsing_enabled = false
  profile()->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, false);
  service_->MaybeLogPasswordReuseDetectedEvent(web_contents());
  ASSERT_EQ(2ul, GetUserEventService()->GetRecordedUserEvents().size());
  event = GetUserEventService()
              ->GetRecordedUserEvents()[1]
              .gaia_password_reuse_event();
  EXPECT_FALSE(event.reuse_detected().status().enabled());

  // Not checking for the extended_reporting_level since that requires setting
  // multiple prefs and doesn't add much verification value.
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyPasswordReuseLookupUserEventRecorded) {
  // Configure sync account type to GMAIL.
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->SetAuthenticatedAccountInfo(kTestAccountID, kTestEmail);
  SetUpSyncAccount(std::string(AccountTrackerService::kNoHostedDomainFound),
                   std::string(kTestAccountID), std::string(kTestEmail));
  EXPECT_EQ(LoginReputationClientRequest::PasswordReuseEvent::GMAIL,
            service_->GetSyncAccountType());

  NavigateAndCommit(GURL("https://www.example.com/"));

  unsigned long t = 0;
  for (const auto& it : kTestCasesWithoutVerdict) {
    service_->MaybeLogPasswordReuseLookupEvent(web_contents(),
                                               it.request_outcome, nullptr);
    ASSERT_EQ(t + 1, GetUserEventService()->GetRecordedUserEvents().size())
        << t;
    PasswordReuseLookup reuse_lookup = GetUserEventService()
                                           ->GetRecordedUserEvents()[t]
                                           .gaia_password_reuse_event()
                                           .reuse_lookup();
    EXPECT_EQ(it.lookup_result, reuse_lookup.lookup_result()) << t;
    t++;
  }

  {
    auto response = std::make_unique<LoginReputationClientResponse>();
    response->set_verdict_token("token1");
    response->set_verdict_type(LoginReputationClientResponse::LOW_REPUTATION);
    service_->MaybeLogPasswordReuseLookupEvent(
        web_contents(), PasswordProtectionService::RESPONSE_ALREADY_CACHED,
        response.get());
    ASSERT_EQ(t + 1, GetUserEventService()->GetRecordedUserEvents().size())
        << t;
    PasswordReuseLookup reuse_lookup = GetUserEventService()
                                           ->GetRecordedUserEvents()[t]
                                           .gaia_password_reuse_event()
                                           .reuse_lookup();
    EXPECT_EQ(PasswordReuseLookup::CACHE_HIT, reuse_lookup.lookup_result())
        << t;
    EXPECT_EQ(PasswordReuseLookup::LOW_REPUTATION, reuse_lookup.verdict()) << t;
    EXPECT_EQ("token1", reuse_lookup.verdict_token()) << t;
    t++;
  }

  {
    auto response = std::make_unique<LoginReputationClientResponse>();
    response->set_verdict_token("token2");
    response->set_verdict_type(LoginReputationClientResponse::SAFE);
    service_->MaybeLogPasswordReuseLookupEvent(
        web_contents(), PasswordProtectionService::SUCCEEDED, response.get());
    ASSERT_EQ(t + 1, GetUserEventService()->GetRecordedUserEvents().size())
        << t;
    PasswordReuseLookup reuse_lookup = GetUserEventService()
                                           ->GetRecordedUserEvents()[t]
                                           .gaia_password_reuse_event()
                                           .reuse_lookup();
    EXPECT_EQ(PasswordReuseLookup::REQUEST_SUCCESS,
              reuse_lookup.lookup_result())
        << t;
    EXPECT_EQ(PasswordReuseLookup::SAFE, reuse_lookup.verdict()) << t;
    EXPECT_EQ("token2", reuse_lookup.verdict_token()) << t;
    t++;
  }
}

TEST_F(ChromePasswordProtectionServiceTest, VerifyGetChangePasswordURL) {
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->SetAuthenticatedAccountInfo(kTestAccountID, kTestEmail);
  SetUpSyncAccount("example.com", std::string(kTestAccountID),
                   std::string(kTestEmail));
  EXPECT_EQ(GURL("https://accounts.google.com/"
                 "AccountChooser?Email=foo%40example.com&continue=https%3A%2F%"
                 "2Fmyaccount.google.com%2Fsigninoptions%2Fpassword%3Futm_"
                 "source%3DGoogle%26utm_campaign%3DPhishGuard&hl=en"),
            service_->GetChangePasswordURL());
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyNavigationDuringPasswordOnFocusPingNotBlocked) {
  GURL trigger_url(kPhishingURL);
  NavigateAndCommit(trigger_url);
  PrepareRequest(LoginReputationClientRequest::UNFAMILIAR_LOGIN_PAGE,
                 /*is_warning_showing=*/false);
  GURL redirect_url(kRedirectURL);
  std::unique_ptr<content::NavigationHandle> test_handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(redirect_url,
                                                                  main_rfh());
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            SimulateWillStart(test_handle.get()));
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyNavigationDuringPasswordReusePingDeferred) {
  GURL trigger_url(kPhishingURL);
  NavigateAndCommit(trigger_url);
  // Simulate a on-going password reuse request that hasn't received
  // verdict yet.
  PrepareRequest(LoginReputationClientRequest::PASSWORD_REUSE_EVENT,
                 /*is_warning_showing=*/false);

  GURL redirect_url(kRedirectURL);
  std::unique_ptr<content::NavigationHandle> test_handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(redirect_url,
                                                                  main_rfh());
  // Verify navigation get deferred.
  EXPECT_EQ(content::NavigationThrottle::DEFER,
            SimulateWillStart(test_handle.get()));
  EXPECT_FALSE(test_handle->HasCommitted());
  base::RunLoop().RunUntilIdle();

  // Simulate receiving a SAFE verdict.
  SimulateRequestFinished(LoginReputationClientResponse::SAFE);
  base::RunLoop().RunUntilIdle();

  // Verify that navigation can be resumed.
  EXPECT_EQ(content::NavigationThrottle::PROCEED,
            test_handle->CallWillProcessResponseForTesting(
                main_rfh(),
                net::HttpUtil::AssembleRawHeaders(
                    kBasicResponseHeaders, strlen(kBasicResponseHeaders))));
  test_handle->CallDidCommitNavigationForTesting(redirect_url);
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(test_handle->HasCommitted());
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyNavigationDuringModalWarningCanceled) {
  GURL trigger_url(kPhishingURL);
  NavigateAndCommit(trigger_url);
  // Simulate a password reuse request, whose verdict is triggering a modal
  // warning.
  PrepareRequest(LoginReputationClientRequest::PASSWORD_REUSE_EVENT,
                 /*is_warning_showing=*/true);
  base::RunLoop().RunUntilIdle();

  // Simulate receiving a phishing verdict.
  SimulateRequestFinished(LoginReputationClientResponse::PHISHING);
  base::RunLoop().RunUntilIdle();

  GURL redirect_url(kRedirectURL);
  std::unique_ptr<content::NavigationHandle> test_handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(redirect_url,
                                                                  main_rfh());
  // Verify that navigation gets canceled.
  EXPECT_EQ(content::NavigationThrottle::CANCEL,
            SimulateWillStart(test_handle.get()));
  EXPECT_FALSE(test_handle->HasCommitted());
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyNavigationThrottleRemovedWhenNavigationHandleIsGone) {
  GURL trigger_url(kPhishingURL);
  NavigateAndCommit(trigger_url);
  // Simulate a on-going password reuse request that hasn't received
  // verdict yet.
  PrepareRequest(LoginReputationClientRequest::PASSWORD_REUSE_EVENT,
                 /*is_warning_showing=*/false);

  GURL redirect_url(kRedirectURL);
  std::unique_ptr<content::NavigationHandle> test_handle =
      content::NavigationHandle::CreateNavigationHandleForTesting(redirect_url,
                                                                  main_rfh());
  // Verify navigation get deferred.
  EXPECT_EQ(content::NavigationThrottle::DEFER,
            SimulateWillStart(test_handle.get()));

  EXPECT_EQ(1u, GetNumberOfNavigationThrottles());

  // Simulate the deletion of NavigationHandle.
  test_handle.reset();
  base::RunLoop().RunUntilIdle();

  // Expect no navigation throttle kept by |request_|.
  EXPECT_EQ(0u, GetNumberOfNavigationThrottles());

  // Simulate receiving a SAFE verdict.
  SimulateRequestFinished(LoginReputationClientResponse::SAFE);
  base::RunLoop().RunUntilIdle();
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyUnhandledSyncPasswordReuseUponClearHistoryDeletion) {
  ASSERT_EQ(0, GetSizeofUnhandledSyncPasswordReuses());
  GURL url_a("https://www.phishinga.com");
  GURL url_b("https://www.phishingb.com");
  GURL url_c("https://www.phishingc.com");

  DictionaryPrefUpdate update(profile()->GetPrefs(),
                              prefs::kSafeBrowsingUnhandledSyncPasswordReuses);
  update->SetKey(Origin::Create(url_a).Serialize(),
                 base::Value("navigation_id_a"));
  update->SetKey(Origin::Create(url_b).Serialize(),
                 base::Value("navigation_id_b"));
  update->SetKey(Origin::Create(url_c).Serialize(),
                 base::Value("navigation_id_c"));

  // Delete a https://www.phishinga.com URL.
  history::URLRows deleted_urls;
  deleted_urls.push_back(history::URLRow(url_a));
  deleted_urls.push_back(history::URLRow(GURL("https://www.notinlist.com")));

  service_->RemoveUnhandledSyncPasswordReuseOnURLsDeleted(
      /*all_history=*/false, deleted_urls);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, GetSizeofUnhandledSyncPasswordReuses());

  service_->RemoveUnhandledSyncPasswordReuseOnURLsDeleted(
      /*all_history=*/true, {});
  EXPECT_EQ(0, GetSizeofUnhandledSyncPasswordReuses());
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyOnPolicySpecifiedPasswordChangedEvent) {
  TestExtensionEventObserver event_observer(test_event_router_);
  base::test::ScopedFeatureList scoped_features;
  scoped_features.InitAndEnableFeature(kEnterprisePasswordProtectionV1);

  // Preparing sync account.
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->SetAuthenticatedAccountInfo(kTestAccountID, kTestEmail);
  SetUpSyncAccount("example.com", std::string(kTestAccountID),
                   std::string(kTestEmail));

  // Simulates change password.
  service_->OnGaiaPasswordChanged();
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(1, test_event_router_->GetEventCount(
                   OnPolicySpecifiedPasswordChanged::kEventName));

  auto captured_args = event_observer.PassEventArgs().GetList()[0].Clone();
  EXPECT_EQ("foo@example.com", captured_args.GetString());

  // If user is in incognito mode, no event should be sent.
  service_->ConfigService(true /*incognito*/, false /*SBER*/);
  service_->OnGaiaPasswordChanged();
  base::RunLoop().RunUntilIdle();
  // Event count should be unchanged.
  EXPECT_EQ(1, test_event_router_->GetEventCount(
                   OnPolicySpecifiedPasswordChanged::kEventName));
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyOnPolicySpecifiedPasswordReuseDetectedEventForPasswordReuse) {
  TestExtensionEventObserver event_observer(test_event_router_);
  base::test::ScopedFeatureList scoped_features;
  scoped_features.InitAndEnableFeature(kEnterprisePasswordProtectionV1);
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->SetAuthenticatedAccountInfo(kTestAccountID, kTestEmail);
  SetUpSyncAccount("example.com", std::string(kTestAccountID),
                   std::string(kTestEmail));
  profile()->GetPrefs()->SetInteger(prefs::kPasswordProtectionWarningTrigger,
                                    PASSWORD_REUSE);
  NavigateAndCommit(GURL(kPasswordReuseURL));

  service_->MaybeStartProtectedPasswordEntryRequest(
      web_contents(), web_contents()->GetLastCommittedURL(),
      /*maches_sync_password=*/true, {},
      /*password_field_exists =*/true);
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(1, test_event_router_->GetEventCount(
                   OnPolicySpecifiedPasswordReuseDetected::kEventName));
  auto captured_args = event_observer.PassEventArgs().GetList()[0].Clone();
  EXPECT_EQ(kPasswordReuseURL, captured_args.FindKey("url")->GetString());
  EXPECT_EQ("foo@example.com", captured_args.FindKey("userName")->GetString());
  EXPECT_FALSE(captured_args.FindKey("isPhishingUrl")->GetBool());

  // If user is in incognito mode, no event should be sent.
  service_->ConfigService(true /*incognito*/, false /*SBER*/);
  service_->MaybeStartProtectedPasswordEntryRequest(
      web_contents(), web_contents()->GetLastCommittedURL(),
      /*maches_sync_password=*/true, {},
      /*password_field_exists =*/true);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, test_event_router_->GetEventCount(
                   OnPolicySpecifiedPasswordReuseDetected::kEventName));
}

TEST_F(ChromePasswordProtectionServiceTest,
       VerifyOnPolicySpecifiedPasswordReuseDetectedEventForPhishingReuse) {
  TestExtensionEventObserver event_observer(test_event_router_);
  base::test::ScopedFeatureList scoped_features;
  scoped_features.InitAndEnableFeature(kEnterprisePasswordProtectionV1);
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile());
  signin_manager->SetAuthenticatedAccountInfo(kTestAccountID, kTestEmail);
  SetUpSyncAccount("example.com", std::string(kTestAccountID),
                   std::string(kTestEmail));
  profile()->GetPrefs()->SetInteger(prefs::kPasswordProtectionWarningTrigger,
                                    PASSWORD_REUSE);
  NavigateAndCommit(GURL(kPhishingURL));
  service_->OnModalWarningShown(web_contents(), "verdict_token");
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(1, test_event_router_->GetEventCount(
                   OnPolicySpecifiedPasswordReuseDetected::kEventName));
  auto captured_args = event_observer.PassEventArgs().GetList()[0].Clone();
  EXPECT_EQ(kPhishingURL, captured_args.FindKey("url")->GetString());
  EXPECT_EQ("foo@example.com", captured_args.FindKey("userName")->GetString());
  EXPECT_TRUE(captured_args.FindKey("isPhishingUrl")->GetBool());

  // If user is in incognito mode, no event should be sent.
  service_->ConfigService(true /*incognito*/, false /*SBER*/);
  service_->OnModalWarningShown(web_contents(), "verdict_token");
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, test_event_router_->GetEventCount(
                   OnPolicySpecifiedPasswordReuseDetected::kEventName));
}

}  // namespace safe_browsing
