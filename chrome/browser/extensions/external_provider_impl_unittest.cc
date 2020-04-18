// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/external_provider_impl.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_path_override.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/updater/extension_cache_fake.h"
#include "chrome/browser/extensions/updater/extension_updater.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_test_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chromeos/system/fake_statistics_provider.h"
#include "chromeos/system/statistics_provider.h"
#include "components/user_manager/scoped_user_manager.h"
#endif

using ::testing::NotNull;
using ::testing::Return;
using ::testing::_;

namespace extensions {

namespace {

using namespace net::test_server;

const char kManifestPath[] = "/update_manifest";
const char kAppPath[] = "/app.crx";

class ExternalProviderImplTest : public ExtensionServiceTestBase {
 public:
  ExternalProviderImplTest() {}
  ~ExternalProviderImplTest() override {}

  void InitServiceWithExternalProviders() {
#if defined(OS_CHROMEOS)
    user_manager::ScopedUserManager scoped_user_manager(
        std::make_unique<chromeos::FakeChromeUserManager>());
#endif
    InitializeExtensionServiceWithUpdaterAndPrefs();

    service()->updater()->SetExtensionCacheForTesting(
        test_extension_cache_.get());

    // Don't install default apps. Some of the default apps are downloaded from
    // the webstore, ignoring the url we pass to kAppsGalleryUpdateURL, which
    // would cause the external updates to never finish install.
    profile_->GetPrefs()->SetString(prefs::kDefaultApps, "");

    ProviderCollection providers;
    extensions::ExternalProviderImpl::CreateExternalProviders(
        service_, profile_.get(), &providers);

    for (std::unique_ptr<ExternalProviderInterface>& provider : providers)
      service_->AddProviderForTesting(std::move(provider));
  }

  void InitializeExtensionServiceWithUpdaterAndPrefs() {
    ExtensionServiceInitParams params = CreateDefaultInitParams();
    params.autoupdate_enabled = true;
    // Create prefs file to make the profile not new.
    const char prefs[] = "{}";
    EXPECT_EQ(base::WriteFile(params.pref_file, prefs, sizeof(prefs)),
              int(sizeof(prefs)));
    InitializeExtensionService(params);
    service_->updater()->Start();
  }

  // ExtensionServiceTestBase overrides:
  void SetUp() override {
    ExtensionServiceTestBase::SetUp();
    test_server_.reset(new EmbeddedTestServer());

    test_server_->RegisterRequestHandler(
        base::Bind(&ExternalProviderImplTest::HandleRequest,
                   base::Unretained(this)));
    ASSERT_TRUE(test_server_->Start());

    test_extension_cache_.reset(new ExtensionCacheFake());

    extension_test_util::SetGalleryUpdateURL(
        test_server_->GetURL(kManifestPath));
  }

 private:
  std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
    GURL url = test_server_->GetURL(request.relative_url);
    if (url.path() == kManifestPath) {
      std::unique_ptr<BasicHttpResponse> response(new BasicHttpResponse);
      response->set_code(net::HTTP_OK);
      response->set_content(base::StringPrintf(
          "<?xml version='1.0' encoding='UTF-8'?>\n"
          "<gupdate xmlns='http://www.google.com/update2/response' "
              "protocol='2.0'>\n"
          "  <app appid='%s'>\n"
          "    <updatecheck codebase='%s' version='1.0' />\n"
          "  </app>\n"
          "</gupdate>",
          extension_misc::kInAppPaymentsSupportAppId,
          test_server_->GetURL(kAppPath).spec().c_str()));
      response->set_content_type("text/xml");
      return std::move(response);
    }
    if (url.path() == kAppPath) {
      base::FilePath test_data_dir;
      base::PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir);
      std::string contents;
      base::ReadFileToString(
          test_data_dir.AppendASCII("extensions/dummyiap.crx"),
          &contents);
      std::unique_ptr<BasicHttpResponse> response(new BasicHttpResponse);
      response->set_code(net::HTTP_OK);
      response->set_content(contents);
      return std::move(response);
    }

    return nullptr;
  }

  std::unique_ptr<EmbeddedTestServer> test_server_;
  std::unique_ptr<ExtensionCacheFake> test_extension_cache_;

#if defined(OS_CHROMEOS)
  // chromeos::ServicesCustomizationExternalLoader is hooked up as an
  // extensions::ExternalLoader and depends on a functioning StatisticsProvider.
  chromeos::system::ScopedFakeStatisticsProvider fake_statistics_provider_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ExternalProviderImplTest);
};

}  // namespace

#if defined(GOOGLE_CHROME_BUILD)
TEST_F(ExternalProviderImplTest, InAppPayments) {
  InitServiceWithExternalProviders();

  scoped_refptr<content::MessageLoopRunner> runner =
      new content::MessageLoopRunner;
  service_->set_external_updates_finished_callback_for_test(
      runner->QuitClosure());

  service_->CheckForExternalUpdates();
  runner->Run();

  EXPECT_TRUE(service_->GetInstalledExtension(
      extension_misc::kInAppPaymentsSupportAppId));
  EXPECT_TRUE(service_->IsExtensionEnabled(
      extension_misc::kInAppPaymentsSupportAppId));
}
#endif  // defined(GOOGLE_CHROME_BUILD)

}  // namespace extensions
