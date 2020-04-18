// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/device_log_ui.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/values.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "components/device_event_log/device_event_log.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace chromeos {

namespace {

class DeviceLogMessageHandler : public content::WebUIMessageHandler {
 public:
  DeviceLogMessageHandler() {}
  ~DeviceLogMessageHandler() override {}

  // WebUIMessageHandler implementation.
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "DeviceLog.getLog",
        base::BindRepeating(&DeviceLogMessageHandler::GetLog,
                            base::Unretained(this)));
  }

 private:
  void GetLog(const base::ListValue* value) const {
    base::Value data(device_event_log::GetAsString(
        device_event_log::NEWEST_FIRST, "json", "",
        device_event_log::LOG_LEVEL_DEBUG, 0));
    web_ui()->CallJavascriptFunctionUnsafe("DeviceLogUI.getLogCallback", data);
  }

  DISALLOW_COPY_AND_ASSIGN(DeviceLogMessageHandler);
};

}  // namespace

DeviceLogUI::DeviceLogUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<DeviceLogMessageHandler>());

  content::WebUIDataSource* html =
      content::WebUIDataSource::Create(chrome::kChromeUIDeviceLogHost);

  html->AddLocalizedString("titleText", IDS_DEVICE_LOG_TITLE);
  html->AddLocalizedString("autoRefreshText", IDS_DEVICE_AUTO_REFRESH);
  html->AddLocalizedString("logRefreshText", IDS_DEVICE_LOG_REFRESH);

  html->AddLocalizedString("logLevelShowText", IDS_DEVICE_LOG_LEVEL_SHOW);
  html->AddLocalizedString("logLevelErrorText", IDS_DEVICE_LOG_LEVEL_ERROR);
  html->AddLocalizedString("logLevelUserText", IDS_DEVICE_LOG_LEVEL_USER);
  html->AddLocalizedString("logLevelEventText", IDS_DEVICE_LOG_LEVEL_EVENT);
  html->AddLocalizedString("logLevelDebugText", IDS_DEVICE_LOG_LEVEL_DEBUG);
  html->AddLocalizedString("logLevelFileinfoText", IDS_DEVICE_LOG_FILEINFO);
  html->AddLocalizedString("logLevelTimeDetailText",
                           IDS_DEVICE_LOG_TIME_DETAIL);

  html->AddLocalizedString("logTypeLoginText", IDS_DEVICE_LOG_TYPE_LOGIN);
  html->AddLocalizedString("logTypeNetworkText", IDS_DEVICE_LOG_TYPE_NETWORK);
  html->AddLocalizedString("logTypePowerText", IDS_DEVICE_LOG_TYPE_POWER);
  html->AddLocalizedString("logTypeBluetoothText",
                           IDS_DEVICE_LOG_TYPE_BLUETOOTH);
  html->AddLocalizedString("logTypeUsbText", IDS_DEVICE_LOG_TYPE_USB);
  html->AddLocalizedString("logTypeHidText", IDS_DEVICE_LOG_TYPE_HID);
  html->AddLocalizedString("logTypePrinterText", IDS_DEVICE_LOG_TYPE_PRINTER);

  html->AddLocalizedString("logEntryFormat", IDS_DEVICE_LOG_ENTRY);
  html->SetJsonPath("strings.js");
  html->AddResourcePath("device_log_ui.css", IDR_DEVICE_LOG_UI_CSS);
  html->AddResourcePath("device_log_ui.js", IDR_DEVICE_LOG_UI_JS);
  html->SetDefaultResource(IDR_DEVICE_LOG_UI_HTML);
  html->UseGzip();

  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                html);
}

DeviceLogUI::~DeviceLogUI() {
}

}  // namespace chromeos
