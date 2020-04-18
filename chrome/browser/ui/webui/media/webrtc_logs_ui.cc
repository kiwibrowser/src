// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media/webrtc_logs_ui.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "components/upload_list/upload_list.h"
#include "components/version_info/version_info.h"
#include "components/webrtc_logging/browser/log_list.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/settings/cros_settings.h"
#endif

using content::WebContents;
using content::WebUIMessageHandler;

namespace {

content::WebUIDataSource* CreateWebRtcLogsUIHTMLSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIWebRtcLogsHost);

  source->AddLocalizedString("webrtcLogsTitle", IDS_WEBRTC_LOGS_TITLE);
  source->AddLocalizedString("webrtcLogCountFormat",
                             IDS_WEBRTC_LOGS_LOG_COUNT_BANNER_FORMAT);
  source->AddLocalizedString("webrtcLogHeaderFormat",
                             IDS_WEBRTC_LOGS_LOG_HEADER_FORMAT);
  source->AddLocalizedString("webrtcLogLocalFileLabelFormat",
                             IDS_WEBRTC_LOGS_LOG_LOCAL_FILE_LABEL_FORMAT);
  source->AddLocalizedString("noLocalLogFileMessage",
                             IDS_WEBRTC_LOGS_NO_LOCAL_LOG_FILE_MESSAGE);
  source->AddLocalizedString("webrtcLogUploadTimeFormat",
                             IDS_WEBRTC_LOGS_LOG_UPLOAD_TIME_FORMAT);
  source->AddLocalizedString("webrtcLogReportIdFormat",
                             IDS_WEBRTC_LOGS_LOG_REPORT_ID_FORMAT);
  source->AddLocalizedString("bugLinkText", IDS_WEBRTC_LOGS_BUG_LINK_LABEL);
  source->AddLocalizedString("webrtcLogNotUploadedMessage",
                             IDS_WEBRTC_LOGS_LOG_NOT_UPLOADED_MESSAGE);
  source->AddLocalizedString("noLogsMessage",
                             IDS_WEBRTC_LOGS_NO_LOGS_MESSAGE);
  source->SetJsonPath("strings.js");
  source->AddResourcePath("webrtc_logs.js", IDR_WEBRTC_LOGS_JS);
  source->SetDefaultResource(IDR_WEBRTC_LOGS_HTML);
  return source;
}

////////////////////////////////////////////////////////////////////////////////
//
// WebRtcLogsDOMHandler
//
////////////////////////////////////////////////////////////////////////////////

// The handler for Javascript messages for the chrome://webrtc-logs/ page.
class WebRtcLogsDOMHandler final : public WebUIMessageHandler {
 public:
  explicit WebRtcLogsDOMHandler(Profile* profile);
  ~WebRtcLogsDOMHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  // Asynchronously fetches the list of upload WebRTC logs. Called from JS.
  void HandleRequestWebRtcLogs(const base::ListValue* args);

  // Sends the recently uploaded logs list JS.
  void UpdateUI();

  // Loads, parses and stores the list of uploaded WebRTC logs.
  scoped_refptr<UploadList> upload_list_;

  // The directory where the logs are stored.
  base::FilePath log_dir_;

  // Factory for creating weak references to instances of this class.
  base::WeakPtrFactory<WebRtcLogsDOMHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcLogsDOMHandler);
};

WebRtcLogsDOMHandler::WebRtcLogsDOMHandler(Profile* profile)
    : log_dir_(
          webrtc_logging::LogList::GetWebRtcLogDirectoryForBrowserContextPath(
              profile->GetPath())),
      weak_ptr_factory_(this) {
  upload_list_ = webrtc_logging::LogList::CreateWebRtcLogList(profile);
}

WebRtcLogsDOMHandler::~WebRtcLogsDOMHandler() {
  upload_list_->CancelCallback();
}

void WebRtcLogsDOMHandler::RegisterMessages() {
  upload_list_->Load(base::BindOnce(&WebRtcLogsDOMHandler::UpdateUI,
                                    weak_ptr_factory_.GetWeakPtr()));

  web_ui()->RegisterMessageCallback(
      "requestWebRtcLogsList",
      base::BindRepeating(&WebRtcLogsDOMHandler::HandleRequestWebRtcLogs,
                          weak_ptr_factory_.GetWeakPtr()));
}

void WebRtcLogsDOMHandler::HandleRequestWebRtcLogs(
    const base::ListValue* args) {
  upload_list_->Load(base::BindOnce(&WebRtcLogsDOMHandler::UpdateUI,
                                    weak_ptr_factory_.GetWeakPtr()));
}

void WebRtcLogsDOMHandler::UpdateUI() {
  std::vector<UploadList::UploadInfo> uploads;
  upload_list_->GetUploads(50, &uploads);

  base::ListValue upload_list;
  for (std::vector<UploadList::UploadInfo>::iterator i = uploads.begin();
       i != uploads.end();
       ++i) {
    std::unique_ptr<base::DictionaryValue> upload(new base::DictionaryValue());
    upload->SetString("id", i->upload_id);

    base::string16 value_w;
    if (!i->upload_time.is_null())
      value_w = base::TimeFormatFriendlyDateAndTime(i->upload_time);
    upload->SetString("upload_time", value_w);

    base::FilePath::StringType value;
    if (!i->local_id.empty())
      value = log_dir_.AppendASCII(i->local_id)
          .AddExtension(FILE_PATH_LITERAL(".gz")).value();
    upload->SetString("local_file", value);

    // In october 2015, capture time was added to the log list, previously the
    // local ID was used as capture time. The local ID has however changed so
    // that it might not be a time. We fall back on the local ID if it traslates
    // to a time within reasonable bounds, otherwise we fall back on the upload
    // time.
    // TODO(grunell): Use |capture_time| only.
    if (!i->capture_time.is_null()) {
      value_w = base::TimeFormatFriendlyDateAndTime(i->capture_time);
    } else {
      // Fall back on local ID as time. We need to check that it's within
      // resonable bounds, since the ID may not represent time. Check between
      // 2012 when the feature was introduced and now.
      double seconds_since_epoch;
      if (base::StringToDouble(i->local_id, &seconds_since_epoch)) {
        base::Time capture_time = base::Time::FromDoubleT(seconds_since_epoch);
        const base::Time::Exploded lower_limit = {2012, 1, 0, 1, 0, 0, 0, 0};
        base::Time out_time;
        bool conversion_success =
            base::Time::FromUTCExploded(lower_limit, &out_time);
        DCHECK(conversion_success);
        if (capture_time > out_time && capture_time < base::Time::Now()) {
          value_w = base::TimeFormatFriendlyDateAndTime(capture_time);
        }
      }
    }
    // If we haven't set |value_w| above, we fall back on the upload time, which
    // was already in the variable. In case it's empty set the string to
    // inform that the time is unknown.
    if (value_w.empty())
      value_w = base::string16(base::ASCIIToUTF16("(unknown time)"));
    upload->SetString("capture_time", value_w);

    upload_list.Append(std::move(upload));
  }

  base::Value version(version_info::GetVersionNumber());

  web_ui()->CallJavascriptFunctionUnsafe("updateWebRtcLogsList", upload_list,
                                         version);
}

}  // namespace

///////////////////////////////////////////////////////////////////////////////
//
// WebRtcLogsUI
//
///////////////////////////////////////////////////////////////////////////////

WebRtcLogsUI::WebRtcLogsUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  web_ui->AddMessageHandler(std::make_unique<WebRtcLogsDOMHandler>(profile));

  // Set up the chrome://webrtc-logs/ source.
  content::WebUIDataSource::Add(profile, CreateWebRtcLogsUIHTMLSource());
}
