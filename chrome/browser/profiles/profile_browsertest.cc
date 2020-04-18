// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile.h"

#include <stddef.h>

#include <memory>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "base/version.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/browser/profiles/chrome_version_service.h"
#include "chrome/browser/profiles/profile_impl.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/bookmarks/browser/startup_task_runner_service.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/net_buildflags.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "services/network/public/cpp/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chromeos/chromeos_switches.h"
#endif

namespace {

// Simple URLFetcherDelegate with an expected final status and the ability to
// wait until a request completes. It's not considered a failure for the request
// to never complete.
class TestURLFetcherDelegate : public net::URLFetcherDelegate {
 public:
  // Creating the TestURLFetcherDelegate automatically creates and starts a
  // URLFetcher.
  TestURLFetcherDelegate(
      scoped_refptr<net::URLRequestContextGetter> context_getter,
      const GURL& url,
      net::URLRequestStatus expected_request_status,
      int load_flags = net::LOAD_NORMAL)
      : expected_request_status_(expected_request_status),
        is_complete_(false),
        fetcher_(net::URLFetcher::Create(url,
                                         net::URLFetcher::GET,
                                         this,
                                         TRAFFIC_ANNOTATION_FOR_TESTS)) {
    fetcher_->SetLoadFlags(load_flags);
    fetcher_->SetRequestContext(context_getter.get());
    fetcher_->Start();
  }

  ~TestURLFetcherDelegate() override {}

  void OnURLFetchComplete(const net::URLFetcher* source) override {
    EXPECT_EQ(expected_request_status_.status(), source->GetStatus().status());
    EXPECT_EQ(expected_request_status_.error(), source->GetStatus().error());

    run_loop_.Quit();
  }

  void WaitForCompletion() {
    run_loop_.Run();
  }

  bool is_complete() const { return is_complete_; }

 private:
  const net::URLRequestStatus expected_request_status_;
  base::RunLoop run_loop_;

  bool is_complete_;
  std::unique_ptr<net::URLFetcher> fetcher_;

  DISALLOW_COPY_AND_ASSIGN(TestURLFetcherDelegate);
};

class MockProfileDelegate : public Profile::Delegate {
 public:
  MOCK_METHOD1(OnPrefsLoaded, void(Profile*));
  MOCK_METHOD3(OnProfileCreated, void(Profile*, bool, bool));
};

// Creates a prefs file in the given directory.
void CreatePrefsFileInDirectory(const base::FilePath& directory_path) {
  base::FilePath pref_path(directory_path.Append(chrome::kPreferencesFilename));
  std::string data("{}");
  ASSERT_EQ(static_cast<int>(data.size()),
            base::WriteFile(pref_path, data.c_str(), data.size()));
}

void CheckChromeVersion(Profile *profile, bool is_new) {
  std::string created_by_version;
  if (is_new) {
    created_by_version = version_info::GetVersionNumber();
  } else {
    created_by_version = "1.0.0.0";
  }
  std::string pref_version =
      ChromeVersionService::GetVersion(profile->GetPrefs());
  // Assert that created_by_version pref gets set to current version.
  EXPECT_EQ(created_by_version, pref_version);
}

void FlushTaskRunner(base::SequencedTaskRunner* runner) {
  ASSERT_TRUE(runner);
  base::WaitableEvent unblock(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                              base::WaitableEvent::InitialState::NOT_SIGNALED);

  runner->PostTask(FROM_HERE, base::BindOnce(&base::WaitableEvent::Signal,
                                             base::Unretained(&unblock)));

  unblock.Wait();
}

void SpinThreads() {
  // Give threads a chance to do their stuff before shutting down (i.e.
  // deleting scoped temp dir etc).
  // Should not be necessary anymore once Profile deletion is fixed
  // (see crbug.com/88586).
  content::RunAllPendingInMessageLoop();

  // This prevents HistoryBackend from accessing its databases after the
  // directory that contains them has been deleted.
  base::TaskScheduler::GetInstance()->FlushForTesting();
}

// Sends an HttpResponse for requests for "/" that result in sending an HPKP
// report.  Ignores other paths to avoid catching the subsequent favicon
// request.
std::unique_ptr<net::test_server::HttpResponse> SendReportHttpResponse(
    const GURL& report_url,
    const net::test_server::HttpRequest& request) {
  if (request.relative_url == "/") {
    std::unique_ptr<net::test_server::BasicHttpResponse> response(
        new net::test_server::BasicHttpResponse());
    std::string header_value = base::StringPrintf(
        "max-age=50000;"
        "pin-sha256=\"9999999999999999999999999999999999999999999=\";"
        "pin-sha256=\"9999999999999999999999999999999999999999998=\";"
        "report-uri=\"%s\"",
        report_url.spec().c_str());
    response->AddCustomHeader("Public-Key-Pins-Report-Only", header_value);
    return std::move(response);
  }

  return nullptr;
}

// Runs |quit_callback| on the UI thread once a URL request has been seen.
// If |hung_response| is true, returns a request that hangs.
std::unique_ptr<net::test_server::HttpResponse> WaitForRequest(
    const base::Closure& quit_closure,
    bool hung_response,
    const net::test_server::HttpRequest& request) {
  // Basic sanity checks on the request.
  EXPECT_EQ("/", request.relative_url);
  EXPECT_EQ("POST", request.method_string);
  base::JSONReader json_reader;
  std::unique_ptr<base::Value> value = json_reader.ReadToValue(request.content);
  EXPECT_TRUE(value);

  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   quit_closure);

  if (hung_response)
    return std::make_unique<net::test_server::HungResponse>();
  return nullptr;
}

// Disables logic to ignore HPKP.  Must be run on IO thread.
void DisablePinningBypass(
    const scoped_refptr<net::URLRequestContextGetter>& getter) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  getter->GetURLRequestContext()
      ->transport_security_state()
      ->SetEnablePublicKeyPinningBypassForLocalTrustAnchors(false);
}

}  // namespace

class ProfileBrowserTest : public InProcessBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
#if defined(OS_CHROMEOS)
    command_line->AppendSwitch(
        chromeos::switches::kIgnoreUserProfileMappingForTests);
#endif
  }

  // content::BrowserTestBase implementation:

  void SetUpOnMainThread() override {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&chrome_browser_net::SetUrlRequestMocksEnabled, true));
  }

  void TearDownOnMainThread() override {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&chrome_browser_net::SetUrlRequestMocksEnabled, false));
  }

  std::unique_ptr<Profile> CreateProfile(const base::FilePath& path,
                                         Profile::Delegate* delegate,
                                         Profile::CreateMode create_mode) {
    std::unique_ptr<Profile> profile(
        Profile::CreateProfile(path, delegate, create_mode));
    EXPECT_TRUE(profile.get());

    // Store the Profile's IO task runner so we can wind it down.
    profile_io_task_runner_ = profile->GetIOTaskRunner();

    return profile;
  }

  void FlushIoTaskRunnerAndSpinThreads() {
    FlushTaskRunner(profile_io_task_runner_.get());
    SpinThreads();
  }

  // Starts a test where a URLFetcher is active during profile shutdown. The
  // test completes during teardown of the test fixture. The request should be
  // canceled by |context_getter| during profile shutdown, before the
  // URLRequestContext is destroyed. If that doesn't happen, the Context's
  // will still have oustanding requests during its destruction, and will
  // trigger a CHECK failure.
  void StartActiveFetcherDuringProfileShutdownTest(
      scoped_refptr<net::URLRequestContextGetter> context_getter) {
    // This method should only be called once per test.
    DCHECK(!url_fetcher_delegate_);

    // Start a hanging request.  This request may or may not completed before
    // the end of the request.
    url_fetcher_delegate_.reset(new TestURLFetcherDelegate(
        context_getter.get(),
        net::URLRequestFailedJob::GetMockHttpUrl(net::ERR_IO_PENDING),
        net::URLRequestStatus(net::URLRequestStatus::CANCELED,
                              net::ERR_CONTEXT_SHUT_DOWN)));

    // Start a second mock request that just fails, and wait for it to complete.
    // This ensures the first request has reached the network stack.
    TestURLFetcherDelegate url_fetcher_delegate2(
        context_getter.get(),
        net::URLRequestFailedJob::GetMockHttpUrl(net::ERR_FAILED),
        net::URLRequestStatus(net::URLRequestStatus::FAILED,
                              net::ERR_FAILED));
    url_fetcher_delegate2.WaitForCompletion();

    // The first request should still be hung.
    EXPECT_FALSE(url_fetcher_delegate_->is_complete());
  }

  // Runs a test where an incognito profile's URLFetcher is active during
  // teardown of the profile, and makes sure the request fails as expected.
  // Also tries issuing a request after the incognito profile has been
  // destroyed.
  static void RunURLFetcherActiveDuringIncognitoTeardownTest(
      Browser* incognito_browser,
      scoped_refptr<net::URLRequestContextGetter> context_getter) {
    // Start a hanging request.
    TestURLFetcherDelegate url_fetcher_delegate1(
        context_getter.get(),
        net::URLRequestFailedJob::GetMockHttpUrl(net::ERR_IO_PENDING),
        net::URLRequestStatus(net::URLRequestStatus::CANCELED,
                              net::ERR_CONTEXT_SHUT_DOWN));

    // Start a second mock request that just fails, and wait for it to complete.
    // This ensures the first request has reached the network stack.
    TestURLFetcherDelegate url_fetcher_delegate2(
        context_getter.get(),
        net::URLRequestFailedJob::GetMockHttpUrl(net::ERR_FAILED),
        net::URLRequestStatus(net::URLRequestStatus::FAILED,
                              net::ERR_FAILED));
    url_fetcher_delegate2.WaitForCompletion();

    // The first request should still be hung.
    EXPECT_FALSE(url_fetcher_delegate1.is_complete());

    // Close all incognito tabs, starting profile shutdown.
    incognito_browser->tab_strip_model()->CloseAllTabs();

    // The request should have been canceled when the Profile shut down.
    url_fetcher_delegate1.WaitForCompletion();

    // Requests issued after Profile shutdown should fail in a similar manner.
    TestURLFetcherDelegate url_fetcher_delegate3(
        context_getter.get(),
        net::URLRequestFailedJob::GetMockHttpUrl(net::ERR_IO_PENDING),
        net::URLRequestStatus(net::URLRequestStatus::CANCELED,
                              net::ERR_CONTEXT_SHUT_DOWN));
    url_fetcher_delegate3.WaitForCompletion();
  }

  scoped_refptr<base::SequencedTaskRunner> profile_io_task_runner_;

  // URLFetcherDelegate that outlives the Profile, to test shutdown.
  std::unique_ptr<TestURLFetcherDelegate> url_fetcher_delegate_;
};

// Test OnProfileCreate is called with is_new_profile set to true when
// creating a new profile synchronously.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, CreateNewProfileSynchronous) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  MockProfileDelegate delegate;
  EXPECT_CALL(delegate, OnProfileCreated(testing::NotNull(), true, true));

  {
    std::unique_ptr<Profile> profile(CreateProfile(
        temp_dir.GetPath(), &delegate, Profile::CREATE_MODE_SYNCHRONOUS));
    CheckChromeVersion(profile.get(), true);

#if defined(OS_CHROMEOS)
    // Make sure session is marked as initialized.
    user_manager::User* user =
        chromeos::ProfileHelper::Get()->GetUserByProfile(profile.get());
    EXPECT_TRUE(user->profile_ever_initialized());
#endif
  }

  FlushIoTaskRunnerAndSpinThreads();
}

// Test OnProfileCreate is called with is_new_profile set to false when
// creating a profile synchronously with an existing prefs file.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, CreateOldProfileSynchronous) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  CreatePrefsFileInDirectory(temp_dir.GetPath());

  MockProfileDelegate delegate;
  EXPECT_CALL(delegate, OnProfileCreated(testing::NotNull(), true, false));

  {
    std::unique_ptr<Profile> profile(CreateProfile(
        temp_dir.GetPath(), &delegate, Profile::CREATE_MODE_SYNCHRONOUS));
    CheckChromeVersion(profile.get(), false);
  }

  FlushIoTaskRunnerAndSpinThreads();
}

// Flaky: http://crbug.com/393177
// Test OnProfileCreate is called with is_new_profile set to true when
// creating a new profile asynchronously.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest,
                       DISABLED_CreateNewProfileAsynchronous) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  MockProfileDelegate delegate;
  EXPECT_CALL(delegate, OnProfileCreated(testing::NotNull(), true, true));

  {
    content::WindowedNotificationObserver observer(
        chrome::NOTIFICATION_PROFILE_CREATED,
        content::NotificationService::AllSources());

    std::unique_ptr<Profile> profile(CreateProfile(
        temp_dir.GetPath(), &delegate, Profile::CREATE_MODE_ASYNCHRONOUS));

    // Wait for the profile to be created.
    observer.Wait();
    CheckChromeVersion(profile.get(), true);
#if defined(OS_CHROMEOS)
    // Make sure session is marked as initialized.
    user_manager::User* user =
        chromeos::ProfileHelper::Get()->GetUserByProfile(profile.get());
    EXPECT_TRUE(user->profile_ever_initialized());
#endif
  }

  FlushIoTaskRunnerAndSpinThreads();
}


// Flaky: http://crbug.com/393177
// Test OnProfileCreate is called with is_new_profile set to false when
// creating a profile asynchronously with an existing prefs file.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest,
                       DISABLED_CreateOldProfileAsynchronous) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  CreatePrefsFileInDirectory(temp_dir.GetPath());

  MockProfileDelegate delegate;
  EXPECT_CALL(delegate, OnProfileCreated(testing::NotNull(), true, false));

  {
    content::WindowedNotificationObserver observer(
        chrome::NOTIFICATION_PROFILE_CREATED,
        content::NotificationService::AllSources());

    std::unique_ptr<Profile> profile(CreateProfile(
        temp_dir.GetPath(), &delegate, Profile::CREATE_MODE_ASYNCHRONOUS));

    // Wait for the profile to be created.
    observer.Wait();
    CheckChromeVersion(profile.get(), false);
  }

  FlushIoTaskRunnerAndSpinThreads();
}

// Flaky: http://crbug.com/393177
// Test that a README file is created for profiles that didn't have it.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, DISABLED_ProfileReadmeCreated) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  MockProfileDelegate delegate;
  EXPECT_CALL(delegate, OnProfileCreated(testing::NotNull(), true, true));

  {
    content::WindowedNotificationObserver observer(
        chrome::NOTIFICATION_PROFILE_CREATED,
        content::NotificationService::AllSources());

    std::unique_ptr<Profile> profile(CreateProfile(
        temp_dir.GetPath(), &delegate, Profile::CREATE_MODE_ASYNCHRONOUS));

    // Wait for the profile to be created.
    observer.Wait();

    // Verify that README exists.
    EXPECT_TRUE(
        base::PathExists(temp_dir.GetPath().Append(chrome::kReadmeFilename)));
  }

  FlushIoTaskRunnerAndSpinThreads();
}

// Test that repeated setting of exit type is handled correctly.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, ExitType) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  MockProfileDelegate delegate;
  EXPECT_CALL(delegate, OnProfileCreated(testing::NotNull(), true, true));
  {
    std::unique_ptr<Profile> profile(CreateProfile(
        temp_dir.GetPath(), &delegate, Profile::CREATE_MODE_SYNCHRONOUS));

    PrefService* prefs = profile->GetPrefs();
    // The initial state is crashed; store for later reference.
    std::string crash_value(prefs->GetString(prefs::kSessionExitType));

    // The first call to a type other than crashed should change the value.
    profile->SetExitType(Profile::EXIT_SESSION_ENDED);
    std::string first_call_value(prefs->GetString(prefs::kSessionExitType));
    EXPECT_NE(crash_value, first_call_value);

    // Subsequent calls to a non-crash value should be ignored.
    profile->SetExitType(Profile::EXIT_NORMAL);
    std::string second_call_value(prefs->GetString(prefs::kSessionExitType));
    EXPECT_EQ(first_call_value, second_call_value);

    // Setting back to a crashed value should work.
    profile->SetExitType(Profile::EXIT_CRASHED);
    std::string final_value(prefs->GetString(prefs::kSessionExitType));
    EXPECT_EQ(crash_value, final_value);
  }

  FlushIoTaskRunnerAndSpinThreads();
}

namespace {

scoped_refptr<const extensions::Extension> BuildTestApp(Profile* profile) {
  scoped_refptr<const extensions::Extension> app;
  app =
      extensions::ExtensionBuilder()
          .SetManifest(
              extensions::DictionaryBuilder()
                  .Set("name", "test app")
                  .Set("version", "1")
                  .Set("app",
                       extensions::DictionaryBuilder()
                           .Set("background",
                                extensions::DictionaryBuilder()
                                    .Set("scripts", extensions::ListBuilder()
                                                        .Append("background.js")
                                                        .Build())
                                    .Build())
                           .Build())
                  .Build())
          .Build();
  extensions::ExtensionRegistry* registry =
      extensions::ExtensionRegistry::Get(profile);
  EXPECT_TRUE(registry->AddEnabled(app));
  return app;
}

void CompareURLRequestContexts(
    net::URLRequestContextGetter* extension_context_getter,
    net::URLRequestContextGetter* main_context_getter) {
  net::URLRequestContext* extension_context =
      extension_context_getter->GetURLRequestContext();
  net::URLRequestContext* main_context =
      main_context_getter->GetURLRequestContext();

  // Check that the URLRequestContexts are different and that their
  // ChannelIDServices, CookieStores, and ReportingServices are different.
  EXPECT_NE(extension_context, main_context);
  EXPECT_NE(extension_context->channel_id_service(),
            main_context->channel_id_service());
  EXPECT_NE(extension_context->cookie_store(), main_context->cookie_store());
#if BUILDFLAG(ENABLE_REPORTING)
  if (extension_context->reporting_service()) {
    EXPECT_NE(extension_context->reporting_service(),
              main_context->reporting_service());
  }
  if (extension_context->network_error_logging_service()) {
    EXPECT_NE(extension_context->network_error_logging_service(),
              main_context->network_error_logging_service());
  }
#endif  // BUILDFLAG(ENABLE_REPORTING)

  // Check that the ChannelIDService in the HttpNetworkSession is the same as
  // the one directly on the URLRequestContext.
  EXPECT_EQ(extension_context->http_transaction_factory()
                ->GetSession()
                ->context()
                .channel_id_service,
            extension_context->channel_id_service());
  EXPECT_EQ(main_context->http_transaction_factory()
                ->GetSession()
                ->context()
                .channel_id_service,
            main_context->channel_id_service());
}

}  // namespace

IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, URLRequestContextIsolation) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

#if BUILDFLAG(ENABLE_REPORTING)
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {network::features::kReporting, network::features::kNetworkErrorLogging},
      {});
#endif  // BUILDFLAG(ENABLE_REPORTING)

  MockProfileDelegate delegate;
  EXPECT_CALL(delegate, OnProfileCreated(testing::NotNull(), true, true));

  {
    std::unique_ptr<Profile> profile(CreateProfile(
        temp_dir.GetPath(), &delegate, Profile::CREATE_MODE_SYNCHRONOUS));

    scoped_refptr<const extensions::Extension> app =
        BuildTestApp(profile.get());
    content::StoragePartition* extension_partition =
        content::BrowserContext::GetStoragePartitionForSite(
            profile.get(),
            extensions::Extension::GetBaseURLFromExtensionId(app->id()));
    net::URLRequestContextGetter* extension_context_getter =
        extension_partition->GetURLRequestContext();
    net::URLRequestContextGetter* main_context_getter =
        profile->GetRequestContext();

    base::RunLoop run_loop;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&CompareURLRequestContexts,
                       base::RetainedRef(extension_context_getter),
                       base::RetainedRef(main_context_getter)),
        run_loop.QuitClosure());
    run_loop.Run();
  }

  FlushIoTaskRunnerAndSpinThreads();
}

IN_PROC_BROWSER_TEST_F(ProfileBrowserTest,
                       OffTheRecordURLRequestContextIsolation) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

#if BUILDFLAG(ENABLE_REPORTING)
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeatures(
      {network::features::kReporting, network::features::kNetworkErrorLogging},
      {});
#endif  // BUILDFLAG(ENABLE_REPORTING)

  MockProfileDelegate delegate;
  EXPECT_CALL(delegate, OnProfileCreated(testing::NotNull(), true, true));

  {
    std::unique_ptr<Profile> profile(CreateProfile(
        temp_dir.GetPath(), &delegate, Profile::CREATE_MODE_SYNCHRONOUS));
    Profile* otr_profile = profile->GetOffTheRecordProfile();

    scoped_refptr<const extensions::Extension> app = BuildTestApp(otr_profile);
    content::StoragePartition* extension_partition =
        content::BrowserContext::GetStoragePartitionForSite(
            otr_profile,
            extensions::Extension::GetBaseURLFromExtensionId(app->id()));
    net::URLRequestContextGetter* extension_context_getter =
        extension_partition->GetURLRequestContext();
    net::URLRequestContextGetter* main_context_getter =
        otr_profile->GetRequestContext();

    base::RunLoop run_loop;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&CompareURLRequestContexts,
                       base::RetainedRef(extension_context_getter),
                       base::RetainedRef(main_context_getter)),
        run_loop.QuitClosure());
    run_loop.Run();
  }

  FlushIoTaskRunnerAndSpinThreads();
}

// The EndSession IO synchronization is only critical on Windows, but also
// happens under the USE_X11 define. See BrowserProcessImpl::EndSession.
#if defined(USE_X11) || defined(OS_WIN) || defined(USE_OZONE)

namespace {

std::string GetExitTypePreferenceFromDisk(Profile* profile) {
  base::FilePath prefs_path =
      profile->GetPath().Append(chrome::kPreferencesFilename);
  std::string prefs;
  if (!base::ReadFileToString(prefs_path, &prefs))
    return std::string();

  std::unique_ptr<base::Value> value = base::JSONReader::Read(prefs);
  if (!value)
    return std::string();

  base::DictionaryValue* dict = NULL;
  if (!value->GetAsDictionary(&dict) || !dict)
    return std::string();

  std::string exit_type;
  if (!dict->GetString("profile.exit_type", &exit_type))
    return std::string();

  return exit_type;
}

}  // namespace

IN_PROC_BROWSER_TEST_F(ProfileBrowserTest,
                       WritesProfilesSynchronouslyOnEndSession) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ASSERT_TRUE(profile_manager);
  std::vector<Profile*> loaded_profiles = profile_manager->GetLoadedProfiles();

  ASSERT_NE(loaded_profiles.size(), 0UL);
  Profile* profile = loaded_profiles[0];

#if defined(OS_CHROMEOS)
  for (auto* loaded_profile : loaded_profiles) {
    if (!chromeos::ProfileHelper::IsSigninProfile(loaded_profile)) {
      profile = loaded_profile;
      break;
    }
  }
#endif

  // This retry loop reduces flakiness due to the fact that this ultimately
  // tests whether or not a code path hits a timed wait.
  bool succeeded = false;
  for (size_t retries = 0; !succeeded && retries < 3; ++retries) {
    // Flush the profile data to disk for all loaded profiles.
    profile->SetExitType(Profile::EXIT_CRASHED);
    profile->GetPrefs()->CommitPendingWrite();
    FlushTaskRunner(profile->GetIOTaskRunner().get());

    // Make sure that the prefs file was written with the expected key/value.
    ASSERT_EQ(GetExitTypePreferenceFromDisk(profile), "Crashed");

    // The blocking wait in EndSession has a timeout.
    base::Time start = base::Time::Now();

    // This must not return until the profile data has been written to disk.
    // If this test flakes, then logoff on Windows has broken again.
    g_browser_process->EndSession();

    base::Time end = base::Time::Now();

    // The EndSession timeout is 10 seconds. If we take more than half that,
    // go around again, as we may have timed out on the wait.
    // This helps against flakes, and also ensures that if the IO thread starts
    // blocking systemically for that length of time (e.g. deadlocking or such),
    // we'll get a consistent test failure.
    if (end - start > base::TimeDelta::FromSeconds(5))
      continue;

    // Make sure that the prefs file was written with the expected key/value.
    ASSERT_EQ(GetExitTypePreferenceFromDisk(profile), "SessionEnded");

    // Mark the success.
    succeeded = true;
  }

  ASSERT_TRUE(succeeded) << "profile->EndSession() timed out too often.";
}

#endif  // defined(USE_X11) || defined(OS_WIN) || defined(USE_OZONE)

// The following tests make sure that it's safe to shut down while one of the
// Profile's URLRequestContextGetters is in use by a URLFetcher.

IN_PROC_BROWSER_TEST_F(ProfileBrowserTest,
                       URLFetcherUsingMainContextDuringShutdown) {
  StartActiveFetcherDuringProfileShutdownTest(
      browser()->profile()->GetRequestContext());
}

IN_PROC_BROWSER_TEST_F(ProfileBrowserTest,
                       URLFetcherUsingMediaContextDuringShutdown) {
  StartActiveFetcherDuringProfileShutdownTest(
      content::BrowserContext::GetDefaultStoragePartition(
          browser()->profile())->GetMediaURLRequestContext());
}

// The following tests make sure that it's safe to destroy an incognito profile
// while one of the its URLRequestContextGetters is in use by a URLFetcher.

IN_PROC_BROWSER_TEST_F(ProfileBrowserTest,
                       URLFetcherUsingMainContextDuringIncognitoTeardown) {
  Browser* incognito_browser =
      OpenURLOffTheRecord(browser()->profile(), GURL("about:blank"));
  RunURLFetcherActiveDuringIncognitoTeardownTest(
      incognito_browser, incognito_browser->profile()->GetRequestContext());
}

// Verifies the cache directory supports multiple profiles when it's overriden
// by group policy or command line switches.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, DiskCacheDirOverride) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  int size;
  const base::FilePath::StringPieceType profile_name =
      FILE_PATH_LITERAL("Profile 1");
  base::ScopedTempDir mock_user_data_dir;
  ASSERT_TRUE(mock_user_data_dir.CreateUniqueTempDir());
  base::FilePath profile_path =
      mock_user_data_dir.GetPath().Append(profile_name);
  ProfileImpl* profile_impl = static_cast<ProfileImpl*>(browser()->profile());

  {
    base::ScopedTempDir temp_disk_cache_dir;
    ASSERT_TRUE(temp_disk_cache_dir.CreateUniqueTempDir());
    profile_impl->GetPrefs()->SetFilePath(prefs::kDiskCacheDir,
                                          temp_disk_cache_dir.GetPath());

    base::FilePath cache_path = profile_path;
    profile_impl->GetMediaCacheParameters(&cache_path, &size);
    EXPECT_EQ(temp_disk_cache_dir.GetPath().Append(profile_name), cache_path);
  }
}

// Test case where an HPKP report is sent.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, SendHPKPReport) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DisablePinningBypass,
          base::WrapRefCounted(browser()->profile()->GetRequestContext())));

  base::RunLoop wait_for_report_loop;
  // Server that HPKP reports are sent to.
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&WaitForRequest, wait_for_report_loop.QuitClosure(), false));
  ASSERT_TRUE(embedded_test_server()->Start());

  // Server that sends an HPKP report when its root document is fetched.
  net::EmbeddedTestServer hpkp_test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  hpkp_test_server.SetSSLConfig(
      net::EmbeddedTestServer::CERT_COMMON_NAME_IS_DOMAIN);
  hpkp_test_server.RegisterRequestHandler(
      base::Bind(&SendReportHttpResponse, embedded_test_server()->base_url()));
  ASSERT_TRUE(hpkp_test_server.Start());

  // To send a report, must use a non-numeric host name for the original
  // request.  This must not match the host name of the server that reports are
  // sent to.
  ui_test_utils::NavigateToURL(browser(),
                               hpkp_test_server.GetURL("localhost", "/"));
  wait_for_report_loop.Run();

  // Shut down the test server, to make it unlikely this will end up in the same
  // situation as the next test, though it's still theoretically possible.
  ASSERT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
}

// Test case where an HPKP report is sent, and the server hasn't replied by the
// time the profile is torn down.  Test will crash if the URLRequestContext is
// torn down before the request is torn down.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, SendHPKPReportServerHangs) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DisablePinningBypass,
          base::WrapRefCounted(browser()->profile()->GetRequestContext())));

  base::RunLoop wait_for_report_loop;
  // Server that HPKP reports are sent to.  Have to use a class member to make
  // sure that the test server outlives the IO thread.
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&WaitForRequest, wait_for_report_loop.QuitClosure(), true));
  ASSERT_TRUE(embedded_test_server()->Start());

  // Server that sends an  HPKP report when its root document is fetched.
  net::EmbeddedTestServer hpkp_test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  hpkp_test_server.SetSSLConfig(
      net::EmbeddedTestServer::CERT_COMMON_NAME_IS_DOMAIN);
  hpkp_test_server.RegisterRequestHandler(
      base::Bind(&SendReportHttpResponse, embedded_test_server()->base_url()));
  ASSERT_TRUE(hpkp_test_server.Start());

  // To send a report, must use a non-numeric host name for the original
  // request.  This must not match the host name of the server that reports are
  // sent to.
  ui_test_utils::NavigateToURL(browser(),
                               hpkp_test_server.GetURL("localhost", "/"));
  wait_for_report_loop.Run();
}

// Verifies the last selected directory has a default value.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, LastSelectedDirectory) {
  ProfileImpl* profile_impl = static_cast<ProfileImpl*>(browser()->profile());
  base::FilePath home;
  base::PathService::Get(base::DIR_HOME, &home);
  ASSERT_EQ(profile_impl->last_selected_directory(), home);
}

// Verifies that, by default, there's a separate disk cache for media files.
IN_PROC_BROWSER_TEST_F(ProfileBrowserTest, SeparateMediaCache) {
  ASSERT_TRUE(embedded_test_server()->Start());

  // Do a normal load using the media URLRequestContext, populating the cache.
  TestURLFetcherDelegate url_fetcher_delegate(
      content::BrowserContext::GetDefaultStoragePartition(browser()->profile())
          ->GetMediaURLRequestContext(),
      embedded_test_server()->GetURL("/cachetime"), net::URLRequestStatus(),
      net::LOAD_NORMAL);
  url_fetcher_delegate.WaitForCompletion();

  // Cache-only load from the main request context should fail, since the media
  // request context has its own cache.
  TestURLFetcherDelegate url_fetcher_delegate2(
      content::BrowserContext::GetDefaultStoragePartition(browser()->profile())
          ->GetURLRequestContext(),
      embedded_test_server()->GetURL("/cachetime"),
      net::URLRequestStatus(net::URLRequestStatus::FAILED, net::ERR_CACHE_MISS),
      net::LOAD_ONLY_FROM_CACHE);
  url_fetcher_delegate2.WaitForCompletion();

  // Cache-only load from the media request context should succeed.
  TestURLFetcherDelegate url_fetcher_delegate3(
      content::BrowserContext::GetDefaultStoragePartition(browser()->profile())
          ->GetMediaURLRequestContext(),
      embedded_test_server()->GetURL("/cachetime"), net::URLRequestStatus(),
      net::LOAD_ONLY_FROM_CACHE);
  url_fetcher_delegate3.WaitForCompletion();
}

class ProfileWithoutMediaCacheBrowserTest : public ProfileBrowserTest {
 public:
  ProfileWithoutMediaCacheBrowserTest() {
    feature_list_.InitAndEnableFeature(features::kUseSameCacheForMedia);
  }

  ~ProfileWithoutMediaCacheBrowserTest() override {}

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Verifies that when kUseSameCacheForMedia is enabled, the media
// URLRequestContext uses the same disk cache as the main one.
IN_PROC_BROWSER_TEST_F(ProfileWithoutMediaCacheBrowserTest,
                       NoSeparateMediaCache) {
  ASSERT_TRUE(embedded_test_server()->Start());

  // Do a normal load using the media URLRequestContext, populating the cache.
  TestURLFetcherDelegate url_fetcher_delegate(
      content::BrowserContext::GetDefaultStoragePartition(browser()->profile())
          ->GetMediaURLRequestContext(),
      embedded_test_server()->GetURL("/cachetime"), net::URLRequestStatus(),
      net::LOAD_NORMAL);
  url_fetcher_delegate.WaitForCompletion();

  // Cache-only load from the main request context should succeed, since the
  // media request context uses the same cache.
  TestURLFetcherDelegate url_fetcher_delegate2(
      content::BrowserContext::GetDefaultStoragePartition(browser()->profile())
          ->GetURLRequestContext(),
      embedded_test_server()->GetURL("/cachetime"), net::URLRequestStatus(),
      net::LOAD_ONLY_FROM_CACHE);
  url_fetcher_delegate2.WaitForCompletion();

  // Cache-only load from the media request context should also succeed.
  TestURLFetcherDelegate url_fetcher_delegate3(
      content::BrowserContext::GetDefaultStoragePartition(browser()->profile())
          ->GetMediaURLRequestContext(),
      embedded_test_server()->GetURL("/cachetime"), net::URLRequestStatus(),
      net::LOAD_ONLY_FROM_CACHE);
  url_fetcher_delegate3.WaitForCompletion();
}
