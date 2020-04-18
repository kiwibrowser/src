// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/system_info_ui.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/feedback/system_logs/about_system_logs_fetcher.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/feedback/system_logs/system_logs_fetcher.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "net/base/directory_lister.h"
#include "net/base/escape.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

using content::WebContents;
using content::WebUIMessageHandler;
using system_logs::SystemLogsResponse;

class SystemInfoUIHTMLSource : public content::URLDataSource{
 public:
  SystemInfoUIHTMLSource();

  // content::URLDataSource implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string&) const override {
    return "text/html";
  }
  std::string GetContentSecurityPolicyScriptSrc() const override {
    // 'unsafe-inline' is added to script-src.
    return "script-src 'self' chrome://resources 'unsafe-eval' "
        "'unsafe-inline';";
  }

  std::string GetContentSecurityPolicyStyleSrc() const override {
    return "style-src 'self' chrome://resources 'unsafe-inline';";
  }

 private:
  ~SystemInfoUIHTMLSource() override {}

  void SysInfoComplete(std::unique_ptr<SystemLogsResponse> response);
  void RequestComplete();
  void WaitForData();

  // Stored data from StartDataRequest()
  std::string path_;
  content::URLDataSource::GotDataCallback callback_;

  std::unique_ptr<SystemLogsResponse> response_;
  base::WeakPtrFactory<SystemInfoUIHTMLSource> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(SystemInfoUIHTMLSource);
};

// The handler for Javascript messages related to the "system" view.
class SystemInfoHandler : public WebUIMessageHandler,
                          public base::SupportsWeakPtr<SystemInfoHandler> {
 public:
  SystemInfoHandler();
  ~SystemInfoHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemInfoHandler);
};

////////////////////////////////////////////////////////////////////////////////
//
// SystemInfoUIHTMLSource
//
////////////////////////////////////////////////////////////////////////////////

SystemInfoUIHTMLSource::SystemInfoUIHTMLSource() : weak_ptr_factory_(this) {}

std::string SystemInfoUIHTMLSource::GetSource() const {
  return chrome::kChromeUISystemInfoHost;
}

void SystemInfoUIHTMLSource::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  path_ = path;
  callback_ = callback;

  system_logs::SystemLogsFetcher* fetcher =
      system_logs::BuildAboutSystemLogsFetcher();
  fetcher->Fetch(base::Bind(&SystemInfoUIHTMLSource::SysInfoComplete,
                            weak_ptr_factory_.GetWeakPtr()));
}

void SystemInfoUIHTMLSource::SysInfoComplete(
    std::unique_ptr<SystemLogsResponse> sys_info) {
  response_ = std::move(sys_info);
  RequestComplete();
}

void SystemInfoUIHTMLSource::RequestComplete() {
  base::DictionaryValue strings;
  strings.SetString("title", l10n_util::GetStringUTF16(IDS_ABOUT_SYS_TITLE));
  strings.SetString("description",
                    l10n_util::GetStringUTF16(IDS_ABOUT_SYS_DESC));
  strings.SetString("tableTitle",
                    l10n_util::GetStringUTF16(IDS_ABOUT_SYS_TABLE_TITLE));
  strings.SetString(
      "logFileTableTitle",
      l10n_util::GetStringUTF16(IDS_ABOUT_SYS_LOG_FILE_TABLE_TITLE));
  strings.SetString("expandAllBtn",
                    l10n_util::GetStringUTF16(IDS_ABOUT_SYS_EXPAND_ALL));
  strings.SetString("collapseAllBtn",
                    l10n_util::GetStringUTF16(IDS_ABOUT_SYS_COLLAPSE_ALL));
  strings.SetString("expandBtn",
                    l10n_util::GetStringUTF16(IDS_ABOUT_SYS_EXPAND));
  strings.SetString("collapseBtn",
                    l10n_util::GetStringUTF16(IDS_ABOUT_SYS_COLLAPSE));
  strings.SetString("parseError",
                    l10n_util::GetStringUTF16(IDS_ABOUT_SYS_PARSE_ERROR));

  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  webui::SetLoadTimeDataDefaults(app_locale, &strings);

  if (response_.get()) {
    auto details = std::make_unique<base::ListValue>();
    for (SystemLogsResponse::const_iterator it = response_->begin();
         it != response_->end();
         ++it) {
      std::unique_ptr<base::DictionaryValue> val(new base::DictionaryValue);
      val->SetString("statName", it->first);
      val->SetString("statValue", it->second);
      details->Append(std::move(val));
    }
    strings.Set("details", std::move(details));
  }
  static const base::StringPiece systeminfo_html(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_ABOUT_SYS_HTML));
  std::string full_html = webui::GetI18nTemplateHtml(systeminfo_html, &strings);
  callback_.Run(base::RefCountedString::TakeString(&full_html));
}

////////////////////////////////////////////////////////////////////////////////
//
// SystemInfoHandler
//
////////////////////////////////////////////////////////////////////////////////
SystemInfoHandler::SystemInfoHandler() {
}

SystemInfoHandler::~SystemInfoHandler() {
}

void SystemInfoHandler::RegisterMessages() {
  // TODO(stevenjb): add message registration, callbacks...
}

////////////////////////////////////////////////////////////////////////////////
//
// SystemInfoUI
//
////////////////////////////////////////////////////////////////////////////////

SystemInfoUI::SystemInfoUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<SystemInfoHandler>());
  SystemInfoUIHTMLSource* html_source = new SystemInfoUIHTMLSource();

  // Set up the chrome://system/ source.
  Profile* profile = Profile::FromWebUI(web_ui);
  content::URLDataSource::Add(profile, html_source);
}
