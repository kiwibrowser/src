// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/metrics/chrome_metrics_service_client.h"
#include "chrome/browser/metrics/chrome_metrics_services_manager_client.h"
#include "chrome/browser/metrics/testing/metrics_reporting_pref_helper.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/test/fake_server/fake_server_network_resources.h"
#include "components/ukm/ukm_service.h"
#include "components/ukm/ukm_source.h"
#include "components/variations/service/variations_field_trial_creator.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browsing_data_remover_test_util.h"
#include "content/public/test/test_utils.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "third_party/metrics_proto/ukm/report.pb.h"
#include "third_party/zlib/google/compression_utils.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/signin_manager_base.h"
#endif

namespace metrics {

namespace {

// Clears the specified data using BrowsingDataRemover.
void ClearBrowsingData(Profile* profile) {
  content::BrowsingDataRemover* remover =
      content::BrowserContext::GetBrowsingDataRemover(profile);
  content::BrowsingDataRemoverCompletionObserver observer(remover);
  remover->RemoveAndReply(
      base::Time(), base::Time::Max(),
      ChromeBrowsingDataRemoverDelegate::DATA_TYPE_HISTORY,
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB, &observer);
  observer.BlockUntilCompletion();
  // Make sure HistoryServiceObservers have a chance to be notified.
  content::RunAllTasksUntilIdle();
}

}  // namespace

// An observer that returns back to test code after a new profile is
// initialized.
void UnblockOnProfileCreation(base::RunLoop* run_loop,
                              Profile* profile,
                              Profile::CreateStatus status) {
  if (status == Profile::CREATE_STATUS_INITIALIZED)
    run_loop->Quit();
}

Profile* CreateNonSyncProfile() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath new_path = profile_manager->GenerateNextProfileDirectoryPath();
  base::RunLoop run_loop;
  profile_manager->CreateProfileAsync(
      new_path, base::Bind(&UnblockOnProfileCreation, &run_loop),
      base::string16(), std::string(), std::string());
  run_loop.Run();
  return profile_manager->GetProfileByPath(new_path);
}

Profile* CreateGuestProfile() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath new_path = profile_manager->GetGuestProfilePath();
  base::RunLoop run_loop;
  profile_manager->CreateProfileAsync(
      new_path, base::Bind(&UnblockOnProfileCreation, &run_loop),
      base::string16(), std::string(), std::string());
  run_loop.Run();
  return profile_manager->GetProfileByPath(new_path);
}

// A helper object for overriding metrics enabled state.
class MetricsConsentOverride {
 public:
  explicit MetricsConsentOverride(bool initial_state) : state_(initial_state) {
    ChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(
        &state_);
    Update(initial_state);
  }

  ~MetricsConsentOverride() {
    ChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(
        nullptr);
  }

  void Update(bool state) {
    state_ = state;
    // Trigger rechecking of metrics state.
    g_browser_process->GetMetricsServicesManager()->UpdateUploadPermissions(
        true);
  }

 private:
  bool state_;
};

// Test fixture that provides access to some UKM internals.
class UkmBrowserTest : public SyncTest {
 public:
  UkmBrowserTest() : SyncTest(SINGLE_CLIENT) {}

  void SetUp() override {
    // Explicitly enable UKM and disable the MetricsReporting (which should
    // not affect UKM).
    scoped_feature_list_.InitWithFeatures({ukm::kUkmFeature},
                                          {internal::kMetricsReportingFeature});
    SyncTest::SetUp();
  }

  bool ukm_enabled() const {
    auto* service = ukm_service();
    return service ? service->recording_enabled_ : false;
  }
  bool ukm_extensions_enabled() const {
    auto* service = ukm_service();
    return service ? service->extensions_enabled_ : false;
  }
  uint64_t client_id() const {
    auto* service = ukm_service();
    DCHECK(service);
    // Can be non-zero only if UpdateUploadPermissions(true) has been called.
    return service->client_id_;
  }
  bool HasDummySource(ukm::SourceId source_id) const {
    auto* service = ukm_service();
    return service ? !!service->sources().count(source_id) : false;
  }
  void RecordDummySource(ukm::SourceId source_id) {
    auto* service = ukm_service();
    if (service)
      service->UpdateSourceURL(source_id, GURL("http://example.com"));
  }
  void BuildAndStoreUkmLog() {
    auto* service = ukm_service();
    DCHECK(service);
    DCHECK(service->initialize_complete_);
    service->Flush();
    DCHECK(service->reporting_service_.ukm_log_store()->has_unsent_logs());
  }
  bool HasUnsentUkmLogs() {
    auto* service = ukm_service();
    DCHECK(service);
    return service->reporting_service_.ukm_log_store()->has_unsent_logs();
  }

  ukm::Report GetUkmReport() {
    EXPECT_TRUE(HasUnsentUkmLogs());

    metrics::PersistedLogs* log_store =
        ukm_service()->reporting_service_.ukm_log_store();
    EXPECT_FALSE(log_store->has_staged_log());
    log_store->StageNextLog();
    EXPECT_TRUE(log_store->has_staged_log());

    std::string uncompressed_log_data;
    EXPECT_TRUE(compression::GzipUncompress(log_store->staged_log(),
                                            &uncompressed_log_data));

    ukm::Report report;
    EXPECT_TRUE(report.ParseFromString(uncompressed_log_data));
    return report;
  }

 protected:
  std::unique_ptr<ProfileSyncServiceHarness> EnableSyncForProfile(
      Profile* profile) {
    browser_sync::ProfileSyncService* sync_service =
        ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile);

    sync_service->OverrideNetworkResourcesForTest(
        std::make_unique<fake_server::FakeServerNetworkResources>(
            GetFakeServer()->AsWeakPtr()));

    std::string username;
    std::string gaia_id;
#if defined(OS_CHROMEOS)
    // In browser tests, the profile may already by authenticated with stub
    // account |user_manager::kStubUserEmail|.
    AccountInfo info = SigninManagerFactory::GetForProfile(profile)
                           ->GetAuthenticatedAccountInfo();
    username = info.email;
    gaia_id = info.gaia;
#endif
    if (username.empty()) {
      username = "user@gmail.com";
      gaia_id = "123456789";
    }

    std::unique_ptr<ProfileSyncServiceHarness> harness =
        ProfileSyncServiceHarness::Create(
            profile, username, gaia_id, "unused" /* password */,
            ProfileSyncServiceHarness::SigninType::FAKE_SIGNIN);
    EXPECT_TRUE(harness->SetupSync());
    return harness;
  }

 private:
  ukm::UkmService* ukm_service() const {
    return g_browser_process->GetMetricsServicesManager()->GetUkmService();
  }
  base::test::ScopedFeatureList scoped_feature_list_;
  DISALLOW_COPY_AND_ASSIGN(UkmBrowserTest);
};

// This tests if UKM service is enabled/disabled appropriately based on an
// input bool param. The bool reflects if metrics reporting state is
// enabled/disabled via prefs.
class UkmConsentParamBrowserTest : public UkmBrowserTest,
                                   public testing::WithParamInterface<bool> {
 public:
  UkmConsentParamBrowserTest() : UkmBrowserTest() {}

  static bool IsMetricsAndCrashReportingEnabled() {
    return ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled();
  }

  // InProcessBrowserTest overrides.
  bool SetUpUserDataDirectory() override {
    local_state_path_ = SetUpUserDataDirectoryForTesting(
        is_metrics_reporting_enabled_initial_value());
    return !local_state_path_.empty();
  }

  void CreatedBrowserMainParts(content::BrowserMainParts* parts) override {
    // IsMetricsReportingEnabled() in non-official builds always returns false.
    // Enable the official build checks so that this test can work in both
    // official and non-official builds.
    ChromeMetricsServiceAccessor::SetForceIsMetricsReportingEnabledPrefLookup(
        true);
  }

  bool is_metrics_reporting_enabled_initial_value() const { return GetParam(); }

 private:
  base::FilePath local_state_path_;
  DISALLOW_COPY_AND_ASSIGN(UkmConsentParamBrowserTest);
};

class UkmEnabledChecker : public SingleClientStatusChangeChecker {
 public:
  UkmEnabledChecker(UkmBrowserTest* test,
                    browser_sync::ProfileSyncService* service,
                    bool want_enabled)
      : SingleClientStatusChangeChecker(service),
        test_(test),
        want_enabled_(want_enabled) {}

  // StatusChangeChecker:
  bool IsExitConditionSatisfied() override {
    return test_->ukm_enabled() == want_enabled_;
  }
  std::string GetDebugMessage() const override {
    return std::string("waiting for ukm_enabled=") +
           (want_enabled_ ? "true" : "false");
  }

 private:
  UkmBrowserTest* const test_;
  const bool want_enabled_;
  DISALLOW_COPY_AND_ASSIGN(UkmEnabledChecker);
};

// Make sure that UKM is disabled while an incognito window is open.
// Keep in sync with UkmTest.testRegularPlusIncognitoCheck in
// chrome/android/javatests/src/org/chromium/chrome/browser/metrics/
// UkmTest.java.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, RegularPlusIncognitoCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  Browser* incognito_browser = CreateIncognitoBrowser();
  EXPECT_FALSE(ukm_enabled());

  // Opening another regular browser mustn't enable UKM.
  Browser* regular_browser = CreateBrowser(profile);
  EXPECT_FALSE(ukm_enabled());

  // Opening and closing another Incognito browser mustn't enable UKM.
  CloseBrowserSynchronously(CreateIncognitoBrowser());
  EXPECT_FALSE(ukm_enabled());

  CloseBrowserSynchronously(regular_browser);
  EXPECT_FALSE(ukm_enabled());

  CloseBrowserSynchronously(incognito_browser);
  EXPECT_TRUE(ukm_enabled());
  // Client ID should not have been reset.
  EXPECT_EQ(original_client_id, client_id());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure opening a real window after Incognito doesn't enable UKM.
// Keep in sync with UkmTest.testIncognitoPlusRegularCheck in
// chrome/android/javatests/src/org/chromium/chrome/browser/metrics/
// UkmTest.java.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, IncognitoPlusRegularCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* incognito_browser = CreateIncognitoBrowser();
  EXPECT_FALSE(ukm_enabled());

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_FALSE(ukm_enabled());

  CloseBrowserSynchronously(incognito_browser);
  EXPECT_TRUE(ukm_enabled());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that UKM is disabled while a guest profile's window is open.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, RegularPlusGuestCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* regular_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();

  // Create browser for guest profile. Only "off the record" browsers may be
  // opened in this mode.
  Profile* guest_profile = CreateGuestProfile();
  Browser* guest_browser = CreateIncognitoBrowser(guest_profile);
  EXPECT_FALSE(ukm_enabled());

  CloseBrowserSynchronously(guest_browser);
  // TODO(crbug/746076): UKM doesn't actually get re-enabled yet.
  // EXPECT_TRUE(ukm_enabled());
  // Client ID should not have been reset.
  EXPECT_EQ(original_client_id, client_id());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(regular_browser);
}

// Make sure that UKM is disabled while an non-sync profile's window is open.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, OpenNonSyncCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  Profile* nonsync_profile = CreateNonSyncProfile();
  Browser* nonsync_browser = CreateBrowser(nonsync_profile);
  EXPECT_FALSE(ukm_enabled());

  CloseBrowserSynchronously(nonsync_browser);
  // TODO(crbug/746076): UKM doesn't actually get re-enabled yet.
  // EXPECT_TRUE(ukm_enabled());
  // Client ID should not have been reset.
  EXPECT_EQ(original_client_id, client_id());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that UKM is disabled when metrics consent is revoked.
// Keep in sync with UkmTest.testMetricConsent in
// chrome/android/sync_shell/javatests/src/org/chromium/chrome/browser/sync/
// UkmTest.java.

IN_PROC_BROWSER_TEST_F(UkmBrowserTest, MetricsConsentCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  // Make sure there is a persistent log.
  BuildAndStoreUkmLog();
  EXPECT_TRUE(HasUnsentUkmLogs());

  metrics_consent.Update(false);
  EXPECT_FALSE(ukm_enabled());
  EXPECT_FALSE(HasUnsentUkmLogs());

  metrics_consent.Update(true);

  EXPECT_TRUE(ukm_enabled());
  // Client ID should have been reset.
  EXPECT_NE(original_client_id, client_id());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

IN_PROC_BROWSER_TEST_F(UkmBrowserTest, LogProtoData) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  // Make sure there is a persistent log.
  BuildAndStoreUkmLog();
  EXPECT_TRUE(HasUnsentUkmLogs());

  // Check log contents.
  ukm::Report report = GetUkmReport();
  EXPECT_EQ(original_client_id, report.client_id());
  // Note: The version number reported in the proto may have a suffix, such as
  // "-64-devel", so use use StartsWith() rather than checking for equality.
  EXPECT_TRUE(base::StartsWith(report.system_profile().app_version(),
                               version_info::GetVersionNumber(),
                               base::CompareCase::SENSITIVE));

// Chrome OS hardware class comes from a different API than on other platforms.
#if defined(OS_CHROMEOS)
  EXPECT_EQ(variations::VariationsFieldTrialCreator::GetShortHardwareClass(),
            report.system_profile().hardware().hardware_class());
#else   // !defined(OS_CHROMEOS)
  EXPECT_EQ(base::SysInfo::HardwareModelName(),
            report.system_profile().hardware().hardware_class());
#endif  // defined(OS_CHROMEOS)

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that providing consent doesn't enable UKM when sync is disabled.
// Keep in sync with UkmTest.consentAddedButNoSyncCheck in
// chrome/android/sync_shell/javatests/src/org/chromium/chrome/browser/sync/
// UkmTest.java.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, ConsentAddedButNoSyncCheck) {
  MetricsConsentOverride metrics_consent(false);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  Browser* browser = CreateBrowser(profile);
  EXPECT_FALSE(ukm_enabled());

  metrics_consent.Update(true);
  EXPECT_FALSE(ukm_enabled());

  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);
  g_browser_process->GetMetricsServicesManager()->UpdateUploadPermissions(true);
  EXPECT_TRUE(ukm_enabled());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(browser);
}

// Make sure that UKM is disabled when an open sync window disables history.
// Keep in sync with UkmTest.singleDisableHistorySyncCheck in
// chrome/android/sync_shell/javatests/src/org/chromium/chrome/browser/sync/
// UkmTest.java.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, SingleDisableHistorySyncCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  harness->DisableSyncForDatatype(syncer::TYPED_URLS);
  EXPECT_FALSE(ukm_enabled());

  harness->EnableSyncForDatatype(syncer::TYPED_URLS);
  EXPECT_TRUE(ukm_enabled());
  // Client ID should be reset.
  EXPECT_NE(original_client_id, client_id());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that UKM is disabled when any open sync window disables history.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, MultiDisableHistorySyncCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile1 = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness1 =
      EnableSyncForProfile(profile1);

  Browser* browser1 = CreateBrowser(profile1);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  Profile* profile2 = CreateNonSyncProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness2 =
      EnableSyncForProfile(profile2);
  Browser* browser2 = CreateBrowser(profile2);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_EQ(original_client_id, client_id());

  harness2->DisableSyncForDatatype(syncer::TYPED_URLS);
  EXPECT_FALSE(ukm_enabled());
  EXPECT_NE(original_client_id, client_id());
  original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  harness2->EnableSyncForDatatype(syncer::TYPED_URLS);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_EQ(original_client_id, client_id());

  harness2->service()->RequestStop(
      browser_sync::ProfileSyncService::CLEAR_DATA);
  harness1->service()->RequestStop(
      browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(browser2);
  CloseBrowserSynchronously(browser1);
}

// Make sure that extension URLs are disabled when an open sync window
// disables it.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, SingleDisableExtensionsSyncCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_TRUE(ukm_extensions_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  harness->DisableSyncForDatatype(syncer::EXTENSIONS);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_FALSE(ukm_extensions_enabled());

  harness->EnableSyncForDatatype(syncer::EXTENSIONS);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_TRUE(ukm_extensions_enabled());
  // Client ID should not be reset.
  EXPECT_EQ(original_client_id, client_id());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that extension URLs are disabled when any open sync window
// disables it.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, MultiDisableExtensionsSyncCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile1 = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness1 =
      EnableSyncForProfile(profile1);

  Browser* browser1 = CreateBrowser(profile1);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  Profile* profile2 = CreateNonSyncProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness2 =
      EnableSyncForProfile(profile2);
  Browser* browser2 = CreateBrowser(profile2);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_TRUE(ukm_extensions_enabled());
  EXPECT_EQ(original_client_id, client_id());

  harness2->DisableSyncForDatatype(syncer::EXTENSIONS);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_FALSE(ukm_extensions_enabled());

  harness2->EnableSyncForDatatype(syncer::EXTENSIONS);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_TRUE(ukm_extensions_enabled());
  EXPECT_EQ(original_client_id, client_id());

  harness2->service()->RequestStop(
      browser_sync::ProfileSyncService::CLEAR_DATA);
  harness1->service()->RequestStop(
      browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(browser2);
  CloseBrowserSynchronously(browser1);
}

// Make sure that UKM is disabled when an secondary passphrase is set.
// Keep in sync with UkmTest.secondaryPassphraseCheck in
// chrome/android/sync_shell/javatests/src/org/chromium/chrome/browser/sync/
// UkmTest.java.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, SecondaryPassphraseCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  // Setting an encryption passphrase is done on the "sync" thread meaning the
  // method only posts a task and returns. That task, when executed, will
  // set the passphrase and notify observers (which disables UKM).
  harness->service()->SetEncryptionPassphrase("foo",
                                              syncer::SyncService::EXPLICIT);
  UkmEnabledChecker checker(this, harness->service(), false);
  EXPECT_TRUE(checker.Wait());

  // Client ID should be reset.
  EXPECT_NE(original_client_id, client_id());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that UKM is disabled when the profile signs out of Sync.
// Keep in sync with UkmTest.singleSyncSignoutCheck in
// chrome/android/sync_shell/javatests/src/org/chromium/chrome/browser/sync/
// UkmTest.java.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, SingleSyncSignoutCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  harness->SignoutSyncService();
  EXPECT_FALSE(ukm_enabled());
  EXPECT_NE(original_client_id, client_id());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that UKM is disabled when any profile signs out of Sync.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, MultiSyncSignoutCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile1 = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness1 =
      EnableSyncForProfile(profile1);

  Browser* browser1 = CreateBrowser(profile1);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  Profile* profile2 = CreateNonSyncProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness2 =
      EnableSyncForProfile(profile2);
  Browser* browser2 = CreateBrowser(profile2);
  EXPECT_TRUE(ukm_enabled());
  EXPECT_EQ(original_client_id, client_id());

  harness2->SignoutSyncService();
  EXPECT_FALSE(ukm_enabled());
  EXPECT_NE(original_client_id, client_id());

  harness2->service()->RequestStop(
      browser_sync::ProfileSyncService::CLEAR_DATA);
  harness1->service()->RequestStop(
      browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(browser2);
  CloseBrowserSynchronously(browser1);
}

// Make sure that if history/sync services weren't available when we tried to
// attach listeners, UKM is not enabled.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, ServiceListenerInitFailedCheck) {
  MetricsConsentOverride metrics_consent(true);
  ChromeMetricsServiceClient::SetNotificationListenerSetupFailedForTesting(
      true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_FALSE(ukm_enabled());
  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that UKM is not affected by MetricsReporting Feature (sampling).
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, MetricsReportingCheck) {
  // Need to set the Metrics Default to OPT_OUT to trigger MetricsReporting.
  DCHECK(g_browser_process);
  PrefService* local_state = g_browser_process->local_state();
  metrics::ForceRecordMetricsReportingDefaultState(
      local_state, metrics::EnableMetricsDefault::OPT_OUT);
  // Verify that kMetricsReportingFeature is disabled (i.e. other metrics
  // services will be sampled out).
  EXPECT_FALSE(
      base::FeatureList::IsEnabled(internal::kMetricsReportingFeature));

  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Make sure that pending data is deleted when user deletes history.
// Keep in sync with UkmTest.testHistoryDeleteCheck in
// chrome/android/javatests/src/org/chromium/chrome/browser/metrics/
// UkmTest.java.
IN_PROC_BROWSER_TEST_F(UkmBrowserTest, HistoryDeleteCheck) {
  MetricsConsentOverride metrics_consent(true);

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);
  EXPECT_TRUE(ukm_enabled());
  uint64_t original_client_id = client_id();
  EXPECT_NE(0U, original_client_id);

  const ukm::SourceId kDummySourceId = 0x54321;
  RecordDummySource(kDummySourceId);
  EXPECT_TRUE(HasDummySource(kDummySourceId));

  ClearBrowsingData(profile);
  // Other sources may already have been recorded since the data was cleared,
  // but the dummy source should be gone.
  EXPECT_FALSE(HasDummySource(kDummySourceId));
  // Client ID should NOT be reset.
  EXPECT_EQ(original_client_id, client_id());
  EXPECT_TRUE(ukm_enabled());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

IN_PROC_BROWSER_TEST_P(UkmConsentParamBrowserTest, GroupPolicyConsentCheck) {
  // Note we are not using the synthetic MetricsConsentOverride since we are
  // testing directly from prefs.

  Profile* profile = ProfileManager::GetActiveUserProfile();
  std::unique_ptr<ProfileSyncServiceHarness> harness =
      EnableSyncForProfile(profile);

  Browser* sync_browser = CreateBrowser(profile);

  // The input param controls whether we set the prefs related to group policy
  // enabled or not. Based on its value, we should report the same value for
  // both if reporting is enabled and if UKM service is enabled.
  bool is_enabled = is_metrics_reporting_enabled_initial_value();
  EXPECT_EQ(is_enabled,
            UkmConsentParamBrowserTest::IsMetricsAndCrashReportingEnabled());
  EXPECT_EQ(is_enabled, ukm_enabled());

  harness->service()->RequestStop(browser_sync::ProfileSyncService::CLEAR_DATA);
  CloseBrowserSynchronously(sync_browser);
}

// Verify UKM is enabled/disabled for both potential settings of group policy.
INSTANTIATE_TEST_CASE_P(UkmConsentParamBrowserTests,
                        UkmConsentParamBrowserTest,
                        testing::Bool());

}  // namespace metrics
