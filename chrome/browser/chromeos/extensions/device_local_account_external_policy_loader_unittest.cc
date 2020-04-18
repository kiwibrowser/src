// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/device_local_account_external_policy_loader.h"

#include <string>
#include <utility>

#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "base/version.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/external_provider_impl.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/policy/core/common/cloud/mock_cloud_policy_store.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/external_install_info.h"
#include "extensions/browser/external_provider_interface.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/updater/extension_downloader.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/manifest.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#endif  // defined(OS_CHROMEOS)

using ::testing::Field;
using ::testing::InvokeWithoutArgs;
using ::testing::Mock;
using ::testing::StrEq;
using ::testing::_;
using extensions::ExternalInstallInfoFile;
using extensions::ExternalInstallInfoUpdateUrl;

namespace chromeos {

namespace {

const char kCacheDir[] = "cache";
const char kExtensionId[] = "ldnnhddmnhbkjipkidpdiheffobcpfmf";
const char kExtensionUpdateManifest[] =
    "extensions/good_v1_update_manifest.xml";
const char kExtensionCRXSourceDir[] = "extensions";
const char kExtensionCRXFile[] = "good.crx";
const char kExtensionCRXVersion[] = "1.0.0.0";

class MockExternalPolicyProviderVisitor
    : public extensions::ExternalProviderInterface::VisitorInterface {
 public:
  MockExternalPolicyProviderVisitor();
  virtual ~MockExternalPolicyProviderVisitor();

  MOCK_METHOD1(OnExternalExtensionFileFound,
               bool(const ExternalInstallInfoFile&));
  MOCK_METHOD2(OnExternalExtensionUpdateUrlFound,
               bool(const ExternalInstallInfoUpdateUrl&, bool));
  MOCK_METHOD1(OnExternalProviderReady,
               void(const extensions::ExternalProviderInterface* provider));
  MOCK_METHOD4(OnExternalProviderUpdateComplete,
               void(const extensions::ExternalProviderInterface*,
                    const std::vector<ExternalInstallInfoUpdateUrl>&,
                    const std::vector<ExternalInstallInfoFile>&,
                    const std::set<std::string>& removed_extensions));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockExternalPolicyProviderVisitor);
};

MockExternalPolicyProviderVisitor::MockExternalPolicyProviderVisitor() {
}

MockExternalPolicyProviderVisitor::~MockExternalPolicyProviderVisitor() {
}

}  // namespace

class DeviceLocalAccountExternalPolicyLoaderTest : public testing::Test {
 protected:
  DeviceLocalAccountExternalPolicyLoaderTest();
  ~DeviceLocalAccountExternalPolicyLoaderTest() override;

  void SetUp() override;
  void TearDown() override;

  void VerifyAndResetVisitorCallExpectations();
  void SetForceInstallListPolicy();

  content::TestBrowserThreadBundle thread_bundle_;
  base::ScopedTempDir temp_dir_;
  base::FilePath cache_dir_;
  policy::MockCloudPolicyStore store_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  base::FilePath test_dir_;

  scoped_refptr<DeviceLocalAccountExternalPolicyLoader> loader_;
  MockExternalPolicyProviderVisitor visitor_;
  std::unique_ptr<extensions::ExternalProviderImpl> provider_;

  content::InProcessUtilityThreadHelper in_process_utility_thread_helper_;

#if defined(OS_CHROMEOS)
  chromeos::ScopedTestDeviceSettingsService test_device_settings_service_;
  chromeos::ScopedTestCrosSettings test_cros_settings_;
#endif // defined(OS_CHROMEOS)
};

DeviceLocalAccountExternalPolicyLoaderTest::
    DeviceLocalAccountExternalPolicyLoaderTest()
    : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP) {
}

DeviceLocalAccountExternalPolicyLoaderTest::
    ~DeviceLocalAccountExternalPolicyLoaderTest() {
}

void DeviceLocalAccountExternalPolicyLoaderTest::SetUp() {
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  cache_dir_ = temp_dir_.GetPath().Append(kCacheDir);
  ASSERT_TRUE(base::CreateDirectoryAndGetError(cache_dir_, NULL));
  request_context_getter_ =
      new net::TestURLRequestContextGetter(base::ThreadTaskRunnerHandle::Get());
  TestingBrowserProcess::GetGlobal()->SetSystemRequestContext(
      request_context_getter_.get());
  ASSERT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &test_dir_));

  loader_ = new DeviceLocalAccountExternalPolicyLoader(&store_, cache_dir_);
  provider_.reset(new extensions::ExternalProviderImpl(
      &visitor_,
      loader_,
      NULL,
      extensions::Manifest::EXTERNAL_POLICY,
      extensions::Manifest::EXTERNAL_POLICY_DOWNLOAD,
      extensions::Extension::NO_FLAGS));

  VerifyAndResetVisitorCallExpectations();
}

void DeviceLocalAccountExternalPolicyLoaderTest::TearDown() {
  TestingBrowserProcess::GetGlobal()->SetSystemRequestContext(NULL);
}

void DeviceLocalAccountExternalPolicyLoaderTest::
    VerifyAndResetVisitorCallExpectations() {
  Mock::VerifyAndClearExpectations(&visitor_);
  EXPECT_CALL(visitor_, OnExternalExtensionFileFound(_)).Times(0);
  EXPECT_CALL(visitor_, OnExternalExtensionUpdateUrlFound(_, _)).Times(0);
  EXPECT_CALL(visitor_, OnExternalProviderReady(_))
      .Times(0);
  EXPECT_CALL(visitor_, OnExternalProviderUpdateComplete(_, _, _, _)).Times(0);
}

void DeviceLocalAccountExternalPolicyLoaderTest::SetForceInstallListPolicy() {
  std::unique_ptr<base::ListValue> forcelist(new base::ListValue);
  forcelist->AppendString("invalid");
  forcelist->AppendString(base::StringPrintf(
      "%s;%s",
      kExtensionId,
      extension_urls::GetWebstoreUpdateUrl().spec().c_str()));
  store_.policy_map_.Set(policy::key::kExtensionInstallForcelist,
                         policy::POLICY_LEVEL_MANDATORY,
                         policy::POLICY_SCOPE_USER, policy::POLICY_SOURCE_CLOUD,
                         std::move(forcelist), nullptr);
  store_.NotifyStoreLoaded();
}

// Verifies that when the cache is not explicitly started, the loader does not
// serve any extensions, even if the force-install list policy is set or a load
// is manually requested.
TEST_F(DeviceLocalAccountExternalPolicyLoaderTest, CacheNotStarted) {
  // Set the force-install list policy.
  SetForceInstallListPolicy();

  // Manually request a load.
  loader_->StartLoading();

  EXPECT_FALSE(loader_->IsCacheRunning());
}

// Verifies that the cache can be started and stopped correctly.
TEST_F(DeviceLocalAccountExternalPolicyLoaderTest, ForceInstallListEmpty) {
  // Set an empty force-install list policy.
  store_.NotifyStoreLoaded();

  // Start the cache. Verify that the loader announces an empty extension list.
  EXPECT_CALL(visitor_, OnExternalProviderReady(provider_.get()))
      .Times(1);
  loader_->StartCache(base::ThreadTaskRunnerHandle::Get());
  base::RunLoop().RunUntilIdle();
  VerifyAndResetVisitorCallExpectations();

  // Stop the cache. Verify that the loader announces an empty extension list.
  EXPECT_CALL(visitor_, OnExternalProviderReady(provider_.get()))
      .Times(1);
  base::RunLoop run_loop;
  loader_->StopCache(run_loop.QuitClosure());
  VerifyAndResetVisitorCallExpectations();

  // Spin the loop until the cache shutdown callback is invoked. Verify that at
  // that point, no further file I/O tasks are pending.
  run_loop.Run();
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
}

// Verifies that when a force-install list policy referencing an extension is
// set and the cache is started, the loader downloads, caches and serves the
// extension.
TEST_F(DeviceLocalAccountExternalPolicyLoaderTest, ForceInstallListSet) {
  // Set a force-install list policy that contains an invalid entry (which
  // should be ignored) and a valid reference to an extension.
  SetForceInstallListPolicy();

  // Start the cache.
  loader_->StartCache(base::ThreadTaskRunnerHandle::Get());

  // Spin the loop, allowing the loader to process the force-install list.
  // Verify that the loader announces an empty extension list.
  net::TestURLFetcherFactory factory;
  EXPECT_CALL(visitor_, OnExternalProviderReady(provider_.get()))
      .Times(1);
  base::RunLoop().RunUntilIdle();

  // Verify that a downloader has started and is attempting to download an
  // update manifest.
  net::TestURLFetcher* fetcher = factory.GetFetcherByID(
      extensions::ExtensionDownloader::kManifestFetcherId);
  ASSERT_TRUE(fetcher);
  ASSERT_TRUE(fetcher->delegate());

  // Return a manifest to the downloader.
  std::string manifest;
  EXPECT_TRUE(base::ReadFileToString(test_dir_.Append(kExtensionUpdateManifest),
                                     &manifest));
  fetcher->set_response_code(200);
  fetcher->SetResponseString(manifest);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Wait for the manifest to be parsed.
  content::WindowedNotificationObserver(
      extensions::NOTIFICATION_EXTENSION_UPDATE_FOUND,
      content::NotificationService::AllSources()).Wait();

  // Verify that the downloader is attempting to download a CRX file.
  fetcher = factory.GetFetcherByID(
      extensions::ExtensionDownloader::kExtensionFetcherId);
  ASSERT_TRUE(fetcher);
  ASSERT_TRUE(fetcher->delegate());

  // Create a temporary CRX file and return its path to the downloader.
  EXPECT_TRUE(base::CopyFile(
      test_dir_.Append(kExtensionCRXSourceDir).Append(kExtensionCRXFile),
      temp_dir_.GetPath().Append(kExtensionCRXFile)));
  fetcher->set_response_code(200);
  fetcher->SetResponseFilePath(temp_dir_.GetPath().Append(kExtensionCRXFile));
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  // Spin the loop. Verify that the loader announces the presence of a new CRX
  // file, served from the cache directory.
  const base::FilePath cached_crx_path = cache_dir_.Append(base::StringPrintf(
      "%s-%s.crx", kExtensionId, kExtensionCRXVersion));
  base::RunLoop cache_run_loop;
  EXPECT_CALL(
      visitor_,
      OnExternalExtensionFileFound(AllOf(
          Field(&extensions::ExternalInstallInfoFile::extension_id,
                StrEq(kExtensionId)),
          Field(&extensions::ExternalInstallInfoFile::path, cached_crx_path),
          Field(&extensions::ExternalInstallInfoFile::crx_location,
                extensions::Manifest::EXTERNAL_POLICY))));
  EXPECT_CALL(visitor_, OnExternalProviderReady(provider_.get()))
      .Times(1)
      .WillOnce(InvokeWithoutArgs(&cache_run_loop, &base::RunLoop::Quit));
  cache_run_loop.Run();
  VerifyAndResetVisitorCallExpectations();

  // Verify that the CRX file actually exists in the cache directory and its
  // contents matches the file returned to the downloader.
  EXPECT_TRUE(base::ContentsEqual(
      test_dir_.Append(kExtensionCRXSourceDir).Append(kExtensionCRXFile),
      cached_crx_path));

  // Stop the cache. Verify that the loader announces an empty extension list.
  EXPECT_CALL(visitor_, OnExternalProviderReady(provider_.get()))
      .Times(1);
  base::RunLoop shutdown_run_loop;
  loader_->StopCache(shutdown_run_loop.QuitClosure());
  VerifyAndResetVisitorCallExpectations();

  // Spin the loop until the cache shutdown callback is invoked. Verify that at
  // that point, no further file I/O tasks are pending.
  shutdown_run_loop.Run();
  EXPECT_TRUE(base::MessageLoopCurrent::Get()->IsIdleForTesting());
}

}  // namespace chromeos
