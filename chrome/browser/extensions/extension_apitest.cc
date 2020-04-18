// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

#include <stddef.h>
#include <utility>

#include "base/base_switches.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/unpacked_installer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/common/content_switches.h"
#include "extensions/browser/api/test/test_api.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/extension_paths.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/switches.h"
#include "extensions/test/result_catcher.h"
#include "net/base/escape.h"
#include "net/base/filename_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/spawned_test_server/spawned_test_server.h"

namespace extensions {

namespace {

const char kTestCustomArg[] = "customArg";
const char kTestDataDirectory[] = "testDataDirectory";
const char kTestWebSocketPort[] = "testWebSocketPort";
const char kFtpServerPort[] = "ftpServer.port";
const char kEmbeddedTestServerPort[] = "testServer.port";
const char kNativeCrxBindingsEnabled[] = "nativeCrxBindingsEnabled";

std::unique_ptr<net::test_server::HttpResponse> HandleServerRedirectRequest(
    const net::test_server::HttpRequest& request) {
  if (!base::StartsWith(request.relative_url, "/server-redirect?",
                        base::CompareCase::SENSITIVE))
    return nullptr;

  size_t query_string_pos = request.relative_url.find('?');
  std::string redirect_target =
      request.relative_url.substr(query_string_pos + 1);

  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse);
  http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
  http_response->AddCustomHeader("Location", redirect_target);
  return std::move(http_response);
}

std::unique_ptr<net::test_server::HttpResponse> HandleEchoHeaderRequest(
    const net::test_server::HttpRequest& request) {
  if (!base::StartsWith(request.relative_url, "/echoheader?",
                        base::CompareCase::SENSITIVE))
    return nullptr;

  size_t query_string_pos = request.relative_url.find('?');
  std::string header_name =
      request.relative_url.substr(query_string_pos + 1);

  std::string header_value;
  auto it = request.headers.find(header_name);
  if (it != request.headers.end())
    header_value = it->second;

  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse);
  http_response->set_code(net::HTTP_OK);
  http_response->set_content(header_value);
  return std::move(http_response);
}

std::unique_ptr<net::test_server::HttpResponse> HandleSetCookieRequest(
    const net::test_server::HttpRequest& request) {
  if (!base::StartsWith(request.relative_url, "/set-cookie?",
                        base::CompareCase::SENSITIVE))
    return nullptr;

  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse);
  http_response->set_code(net::HTTP_OK);

  size_t query_string_pos = request.relative_url.find('?');
  std::string cookie_value =
      request.relative_url.substr(query_string_pos + 1);

  for (const std::string& cookie : base::SplitString(
           cookie_value, "&", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL))
    http_response->AddCustomHeader("Set-Cookie", cookie);

  return std::move(http_response);
}

std::unique_ptr<net::test_server::HttpResponse> HandleSetHeaderRequest(
    const net::test_server::HttpRequest& request) {
  if (!base::StartsWith(request.relative_url, "/set-header?",
                        base::CompareCase::SENSITIVE))
    return nullptr;

  size_t query_string_pos = request.relative_url.find('?');
  std::string escaped_header =
      request.relative_url.substr(query_string_pos + 1);

  std::string header = net::UnescapeURLComponent(
      escaped_header,
      net::UnescapeRule::NORMAL | net::UnescapeRule::SPACES |
          net::UnescapeRule::PATH_SEPARATORS |
          net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS);

  size_t colon_pos = header.find(':');
  if (colon_pos == std::string::npos)
    return std::unique_ptr<net::test_server::HttpResponse>();

  std::string header_name = header.substr(0, colon_pos);
  // Skip space after colon.
  std::string header_value = header.substr(colon_pos + 2);

  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse);
  http_response->set_code(net::HTTP_OK);
  http_response->AddCustomHeader(header_name, header_value);
  return std::move(http_response);
}

};  // namespace

ExtensionApiTest::ExtensionApiTest() {
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleServerRedirectRequest));
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleEchoHeaderRequest));
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleSetCookieRequest));
  embedded_test_server()->RegisterRequestHandler(
      base::Bind(&HandleSetHeaderRequest));
}

ExtensionApiTest::~ExtensionApiTest() {}

void ExtensionApiTest::SetUpOnMainThread() {
  ExtensionBrowserTest::SetUpOnMainThread();
  DCHECK(!test_config_.get()) << "Previous test did not clear config state.";
  test_config_.reset(new base::DictionaryValue());
  test_config_->SetString(kTestDataDirectory,
                          net::FilePathToFileURL(test_data_dir_).spec());
  if (embedded_test_server()->Started()) {
    // InitializeEmbeddedTestServer was called before |test_config_| was set.
    // Set the missing port key.
    test_config_->SetInteger(kEmbeddedTestServerPort,
                             embedded_test_server()->port());
  }
  test_config_->SetBoolean(
      kNativeCrxBindingsEnabled,
      base::FeatureList::IsEnabled(features::kNativeCrxBindings));
  TestGetConfigFunction::set_test_config_state(test_config_.get());
}

void ExtensionApiTest::TearDownOnMainThread() {
  ExtensionBrowserTest::TearDownOnMainThread();
  TestGetConfigFunction::set_test_config_state(NULL);
  test_config_.reset(NULL);
}

bool ExtensionApiTest::RunExtensionTest(const std::string& extension_name) {
  return RunExtensionTestImpl(
      extension_name, std::string(), NULL, kFlagEnableFileAccess);
}

bool ExtensionApiTest::RunExtensionTestWithFlags(
    const std::string& extension_name,
    int flags) {
  return RunExtensionTestImpl(extension_name, std::string(), nullptr, flags);
}

bool ExtensionApiTest::RunExtensionTestWithArg(
    const std::string& extension_name,
    const char* custom_arg) {
  return RunExtensionTestImpl(extension_name, std::string(), custom_arg,
                              kFlagEnableFileAccess);
}

bool ExtensionApiTest::RunExtensionTestIncognito(
    const std::string& extension_name) {
  return RunExtensionTestImpl(extension_name,
                              std::string(),
                              NULL,
                              kFlagEnableIncognito | kFlagEnableFileAccess);
}

bool ExtensionApiTest::RunExtensionTestIgnoreManifestWarnings(
    const std::string& extension_name) {
  return RunExtensionTestImpl(
      extension_name, std::string(), NULL, kFlagIgnoreManifestWarnings);
}

bool ExtensionApiTest::RunExtensionTestAllowOldManifestVersion(
    const std::string& extension_name) {
  return RunExtensionTestImpl(
      extension_name,
      std::string(),
      NULL,
      kFlagEnableFileAccess | kFlagAllowOldManifestVersions);
}

bool ExtensionApiTest::RunComponentExtensionTest(
    const std::string& extension_name) {
  return RunExtensionTestImpl(extension_name,
                              std::string(),
                              NULL,
                              kFlagEnableFileAccess | kFlagLoadAsComponent);
}

bool ExtensionApiTest::RunExtensionTestNoFileAccess(
    const std::string& extension_name) {
  return RunExtensionTestImpl(extension_name, std::string(), NULL, kFlagNone);
}

bool ExtensionApiTest::RunExtensionTestIncognitoNoFileAccess(
    const std::string& extension_name) {
  return RunExtensionTestImpl(
      extension_name, std::string(), NULL, kFlagEnableIncognito);
}

bool ExtensionApiTest::ExtensionSubtestsAreSkipped() {
  // See http://crbug.com/177163 for details.
#if defined(OS_WIN) && !defined(NDEBUG)
  LOG(WARNING) << "Workaround for 177163, prematurely returning";
  return true;
#else
  return false;
#endif
}

bool ExtensionApiTest::RunExtensionSubtest(const std::string& extension_name,
                                           const std::string& page_url) {
  return RunExtensionSubtestWithArgAndFlags(extension_name, page_url, nullptr,
                                            kFlagEnableFileAccess);
}

bool ExtensionApiTest::RunExtensionSubtest(const std::string& extension_name,
                                           const std::string& page_url,
                                           int flags) {
  return RunExtensionSubtestWithArgAndFlags(extension_name, page_url, nullptr,
                                            flags);
}

bool ExtensionApiTest::RunExtensionSubtestWithArg(
    const std::string& extension_name,
    const std::string& page_url,
    const char* custom_arg) {
  return RunExtensionSubtestWithArgAndFlags(extension_name, page_url,
                                            custom_arg, kFlagEnableFileAccess);
}

bool ExtensionApiTest::RunExtensionSubtestWithArgAndFlags(
    const std::string& extension_name,
    const std::string& page_url,
    const char* custom_arg,
    int flags) {
  DCHECK(!page_url.empty()) << "Argument page_url is required.";
  if (ExtensionSubtestsAreSkipped())
    return true;
  return RunExtensionTestImpl(extension_name, page_url, custom_arg, flags);
}

bool ExtensionApiTest::RunPageTest(const std::string& page_url) {
  return RunExtensionSubtest(std::string(), page_url);
}

bool ExtensionApiTest::RunPageTest(const std::string& page_url,
                                   int flags) {
  return RunExtensionSubtest(std::string(), page_url, flags);
}

bool ExtensionApiTest::RunPlatformAppTest(const std::string& extension_name) {
  return RunExtensionTestImpl(
      extension_name, std::string(), NULL, kFlagLaunchPlatformApp);
}

bool ExtensionApiTest::RunPlatformAppTestWithArg(
    const std::string& extension_name, const char* custom_arg) {
  return RunPlatformAppTestWithFlags(extension_name, custom_arg, kFlagNone);
}

bool ExtensionApiTest::RunPlatformAppTestWithFlags(
    const std::string& extension_name, int flags) {
  return RunExtensionTestImpl(
      extension_name, std::string(), NULL, flags | kFlagLaunchPlatformApp);
}

bool ExtensionApiTest::RunPlatformAppTestWithFlags(
    const std::string& extension_name,
    const char* custom_arg,
    int flags) {
  return RunExtensionTestImpl(extension_name, std::string(), custom_arg,
                              flags | kFlagLaunchPlatformApp);
}


// Load |extension_name| extension and/or |page_url| and wait for
// PASSED or FAILED notification.
bool ExtensionApiTest::RunExtensionTestImpl(const std::string& extension_name,
                                            const std::string& page_url,
                                            const char* custom_arg,
                                            int flags) {
  bool load_as_component = (flags & kFlagLoadAsComponent) != 0;
  bool launch_platform_app = (flags & kFlagLaunchPlatformApp) != 0;
  bool use_incognito = (flags & kFlagUseIncognito) != 0;
  bool use_root_extensions_dir = (flags & kFlagUseRootExtensionsDir) != 0;

  if (custom_arg && custom_arg[0])
    SetCustomArg(custom_arg);

  ResultCatcher catcher;
  DCHECK(!extension_name.empty() || !page_url.empty()) <<
      "extension_name and page_url cannot both be empty";

  const Extension* extension = NULL;
  if (!extension_name.empty()) {
    const base::FilePath& root_path =
        use_root_extensions_dir ? shared_test_data_dir_ : test_data_dir_;
    base::FilePath extension_path = root_path.AppendASCII(extension_name);
    if (load_as_component) {
      extension = LoadExtensionAsComponent(extension_path);
    } else {
      int browser_test_flags = ExtensionBrowserTest::kFlagNone;
      if (flags & kFlagEnableIncognito)
        browser_test_flags |= ExtensionBrowserTest::kFlagEnableIncognito;
      if (flags & kFlagEnableFileAccess)
        browser_test_flags |= ExtensionBrowserTest::kFlagEnableFileAccess;
      if (flags & kFlagIgnoreManifestWarnings)
        browser_test_flags |= ExtensionBrowserTest::kFlagIgnoreManifestWarnings;
      if (flags & kFlagAllowOldManifestVersions) {
        browser_test_flags |=
            ExtensionBrowserTest::kFlagAllowOldManifestVersions;
      }
      extension = LoadExtensionWithFlags(extension_path, browser_test_flags);
    }
    if (!extension) {
      message_ = "Failed to load extension.";
      return false;
    }
  }

  // If there is a page_url to load, navigate it.
  if (!page_url.empty()) {
    GURL url = GURL(page_url);

    // Note: We use is_valid() here in the expectation that the provided url
    // may lack a scheme & host and thus be a relative url within the loaded
    // extension.
    if (!url.is_valid()) {
      DCHECK(!extension_name.empty()) <<
          "Relative page_url given with no extension_name";

      url = extension->GetResourceURL(page_url);
    }

    if (use_incognito)
      OpenURLOffTheRecord(browser()->profile(), url);
    else
      ui_test_utils::NavigateToURL(browser(), url);
  } else if (launch_platform_app) {
    AppLaunchParams params(browser()->profile(), extension,
                           LAUNCH_CONTAINER_NONE,
                           WindowOpenDisposition::NEW_WINDOW, SOURCE_TEST);
    params.command_line = *base::CommandLine::ForCurrentProcess();
    OpenApplication(params);
  }

  if (!catcher.GetNextResult()) {
    message_ = catcher.message();
    return false;
  }

  return true;
}

// Test that exactly one extension is loaded, and return it.
const Extension* ExtensionApiTest::GetSingleLoadedExtension() {
  ExtensionRegistry* registry = ExtensionRegistry::Get(browser()->profile());

  const Extension* result = NULL;
  for (const scoped_refptr<const Extension>& extension :
       registry->enabled_extensions()) {
    // Ignore any component extensions. They are automatically loaded into all
    // profiles and aren't the extension we're looking for here.
    if (extension->location() == Manifest::COMPONENT)
      continue;

    if (result != NULL) {
      // TODO(yoz): this is misleading; it counts component extensions.
      message_ = base::StringPrintf(
          "Expected only one extension to be present.  Found %u.",
          static_cast<unsigned>(registry->enabled_extensions().size()));
      return NULL;
    }

    result = extension.get();
  }

  if (!result) {
    message_ = "extension pointer is NULL.";
    return NULL;
  }
  return result;
}

bool ExtensionApiTest::StartEmbeddedTestServer() {
  if (!InitializeEmbeddedTestServer())
    return false;

  EmbeddedTestServerAcceptConnections();
  return true;
}

bool ExtensionApiTest::InitializeEmbeddedTestServer() {
  if (!embedded_test_server()->InitializeAndListen())
    return false;

  // Build a dictionary of values that tests can use to build URLs that
  // access the test server and local file system.  Tests can see these values
  // using the extension API function chrome.test.getConfig().
  if (test_config_) {
    test_config_->SetInteger(kEmbeddedTestServerPort,
                             embedded_test_server()->port());
  }
  // else SetUpOnMainThread has not been called yet. Possibly because the
  // caller needs a valid port in an overridden SetUpCommandLine method.

  return true;
}

void ExtensionApiTest::EmbeddedTestServerAcceptConnections() {
  embedded_test_server()->StartAcceptingConnections();
}

bool ExtensionApiTest::StartWebSocketServer(
    const base::FilePath& root_directory,
    bool enable_basic_auth) {
  websocket_server_.reset(new net::SpawnedTestServer(
      net::SpawnedTestServer::TYPE_WS, root_directory));
  websocket_server_->set_websocket_basic_auth(enable_basic_auth);

  if (!websocket_server_->Start())
    return false;

  test_config_->SetInteger(kTestWebSocketPort,
                           websocket_server_->host_port_pair().port());

  return true;
}

bool ExtensionApiTest::StartFTPServer(const base::FilePath& root_directory) {
  ftp_server_.reset(new net::SpawnedTestServer(net::SpawnedTestServer::TYPE_FTP,
                                               root_directory));

  if (!ftp_server_->Start())
    return false;

  test_config_->SetInteger(kFtpServerPort,
                           ftp_server_->host_port_pair().port());

  return true;
}

void ExtensionApiTest::SetCustomArg(base::StringPiece custom_arg) {
  test_config_->SetKey(kTestCustomArg, base::Value(custom_arg));
}

void ExtensionApiTest::SetUpCommandLine(base::CommandLine* command_line) {
  ExtensionBrowserTest::SetUpCommandLine(command_line);

  test_data_dir_ = test_data_dir_.AppendASCII("api_test");

  RegisterPathProvider();
  base::PathService::Get(DIR_TEST_DATA, &shared_test_data_dir_);
  shared_test_data_dir_ = shared_test_data_dir_.AppendASCII("api_test");

  // Backgrounded renderer processes run at a lower priority, causing the
  // tests to take more time to complete. Disable backgrounding so that the
  // tests don't time out.
  command_line->AppendSwitch(::switches::kDisableRendererBackgrounding);
}

}  // namespace extensions
