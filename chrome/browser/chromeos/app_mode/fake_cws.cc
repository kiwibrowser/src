// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/app_mode/fake_cws.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "crypto/sha2.h"
#include "extensions/common/extensions_client.h"
#include "net/base/url_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

using net::test_server::BasicHttpResponse;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;

namespace chromeos {

namespace {

const char kWebstoreDomain[] = "cws.com";
// Kiosk app crx file download path under web store site.
const char kCrxDownloadPath[] = "/chromeos/app_mode/webstore/downloads/";

const char kAppNoUpdateTemplate[] =
    "<app appid=\"$AppId\" status=\"ok\">"
      "<updatecheck status=\"noupdate\"/>"
    "</app>";

const char kAppHasUpdateTemplate[] =
    "<app appid=\"$AppId\" status=\"ok\">"
      "<updatecheck codebase=\"$CrxDownloadUrl\" fp=\"1.$FP\" "
        "hash=\"\" hash_sha256=\"$FP\" size=\"$Size\" status=\"ok\" "
        "version=\"$Version\"/>"
    "</app>";

const char kPrivateStoreAppHasUpdateTemplate[] =
    "<app appid=\"$AppId\">"
      "<updatecheck codebase=\"$CrxDownloadUrl\" version=\"$Version\"/>"
    "</app>";

const char kUpdateContentTemplate[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<gupdate xmlns=\"http://www.google.com/update2/response\" "
        "protocol=\"2.0\" server=\"prod\">"
      "<daystart elapsed_days=\"2569\" elapsed_seconds=\"36478\"/>"
      "$APPS"
    "</gupdate>";

bool GetAppIdsFromUpdateUrl(const GURL& update_url,
                            std::vector<std::string>* ids) {
  for (net::QueryIterator it(update_url); !it.IsAtEnd(); it.Advance()) {
    if (it.GetKey() != "x")
      continue;
    std::string id;
    net::GetValueForKeyInQuery(GURL("http://dummy?" + it.GetUnescapedValue()),
                               "id", &id);
    ids->push_back(id);
  }
  return !ids->empty();
}

}  // namespace

FakeCWS::FakeCWS() : update_check_count_(0) {
}

FakeCWS::~FakeCWS() {
}

void FakeCWS::Init(net::EmbeddedTestServer* embedded_test_server) {
  has_update_template_ = kAppHasUpdateTemplate;
  no_update_template_ = kAppNoUpdateTemplate;
  update_check_end_point_ = "/update_check.xml";

  SetupWebStoreURL(embedded_test_server->base_url());
  OverrideGalleryCommandlineSwitches(GalleryUpdateMode::kOnlyCommandLine);
  embedded_test_server->RegisterRequestHandler(
      base::Bind(&FakeCWS::HandleRequest, base::Unretained(this)));
}

void FakeCWS::InitAsPrivateStore(net::EmbeddedTestServer* embedded_test_server,
                                 const std::string& update_check_end_point) {
  has_update_template_ = kPrivateStoreAppHasUpdateTemplate;
  no_update_template_ = kAppNoUpdateTemplate;
  update_check_end_point_ = update_check_end_point;

  SetupWebStoreURL(embedded_test_server->base_url());
  OverrideGalleryCommandlineSwitches(
      GalleryUpdateMode::kModifyExtensionsClient);

  embedded_test_server->RegisterRequestHandler(
      base::Bind(&FakeCWS::HandleRequest, base::Unretained(this)));
}

void FakeCWS::SetUpdateCrx(const std::string& app_id,
                           const std::string& crx_file,
                           const std::string& version) {
  GURL crx_download_url = web_store_url_.Resolve(kCrxDownloadPath + crx_file);

  base::FilePath test_data_dir;
  base::PathService::Get(chrome::DIR_TEST_DATA, &test_data_dir);
  base::FilePath crx_file_path =
      test_data_dir.AppendASCII("chromeos/app_mode/webstore/downloads")
          .AppendASCII(crx_file);
  std::string crx_content;
  ASSERT_TRUE(base::ReadFileToString(crx_file_path, &crx_content));

  const std::string sha256 = crypto::SHA256HashString(crx_content);
  const std::string sha256_hex = base::HexEncode(sha256.c_str(), sha256.size());

  std::string update_check_content(has_update_template_);
  base::ReplaceSubstringsAfterOffset(&update_check_content, 0, "$AppId",
                                     app_id);
  base::ReplaceSubstringsAfterOffset(
      &update_check_content, 0, "$CrxDownloadUrl", crx_download_url.spec());
  base::ReplaceSubstringsAfterOffset(&update_check_content, 0, "$FP",
                                     sha256_hex);
  base::ReplaceSubstringsAfterOffset(&update_check_content, 0, "$Size",
                                     base::UintToString(crx_content.size()));
  base::ReplaceSubstringsAfterOffset(&update_check_content, 0, "$Version",
                                     version);
  id_to_update_check_content_map_[app_id] = update_check_content;
}

void FakeCWS::SetNoUpdate(const std::string& app_id) {
  std::string app_update_check_content(no_update_template_);
  base::ReplaceSubstringsAfterOffset(&app_update_check_content, 0, "$AppId",
                                     app_id);
  id_to_update_check_content_map_[app_id] = app_update_check_content;
}

int FakeCWS::GetUpdateCheckCountAndReset() {
  int current_count = update_check_count_;
  update_check_count_ = 0;
  return current_count;
}

void FakeCWS::SetupWebStoreURL(const GURL& test_server_url) {
  GURL::Replacements replace_webstore_host;
  replace_webstore_host.SetHostStr(kWebstoreDomain);
  web_store_url_ = test_server_url.ReplaceComponents(replace_webstore_host);
}

void FakeCWS::OverrideGalleryCommandlineSwitches(
    GalleryUpdateMode gallery_update_mode) {
  DCHECK(web_store_url_.is_valid());

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  command_line->AppendSwitchASCII(
      ::switches::kAppsGalleryURL,
      web_store_url_.Resolve("/chromeos/app_mode/webstore").spec());

  std::string downloads_path = std::string(kCrxDownloadPath).append("%s.crx");
  GURL downloads_url = web_store_url_.Resolve(downloads_path);
  command_line->AppendSwitchASCII(::switches::kAppsGalleryDownloadURL,
                                  downloads_url.spec());

  GURL update_url = web_store_url_.Resolve(update_check_end_point_);
  command_line->AppendSwitchASCII(::switches::kAppsGalleryUpdateURL,
                                  update_url.spec());

  if (gallery_update_mode == GalleryUpdateMode::kModifyExtensionsClient)
    extensions::ExtensionsClient::Get()->InitializeWebStoreUrls(command_line);
}

bool FakeCWS::GetUpdateCheckContent(const std::vector<std::string>& ids,
                                    std::string* update_check_content) {
  std::string apps_content;
  for (const std::string& id : ids) {
    std::string app_update_content;
    auto it = id_to_update_check_content_map_.find(id);
    if (it == id_to_update_check_content_map_.end())
      return false;
    apps_content.append(it->second);
  }
  if (apps_content.empty())
    return false;

  *update_check_content = kUpdateContentTemplate;
  base::ReplaceSubstringsAfterOffset(update_check_content, 0, "$APPS",
                                     apps_content);
  return true;
}

std::unique_ptr<HttpResponse> FakeCWS::HandleRequest(
    const HttpRequest& request) {
  GURL request_url = GURL("http://localhost").Resolve(request.relative_url);
  std::string request_path = request_url.path();
  if (request_path.find(update_check_end_point_) != std::string::npos &&
      !id_to_update_check_content_map_.empty()) {
    std::vector<std::string> ids;
    if (GetAppIdsFromUpdateUrl(request_url, &ids)) {
      std::string update_check_content;
      if (GetUpdateCheckContent(ids, &update_check_content)) {
        ++update_check_count_;
        std::unique_ptr<BasicHttpResponse> http_response(
            new BasicHttpResponse());
        http_response->set_code(net::HTTP_OK);
        http_response->set_content_type("text/xml");
        http_response->set_content(update_check_content);
        return std::move(http_response);
      }
    }
  }

  return std::unique_ptr<HttpResponse>();
}

}  // namespace chromeos
