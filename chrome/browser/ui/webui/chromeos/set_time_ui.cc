// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/set_time_ui.h"

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/build_time.h"
#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/system_clock_client.h"
#include "chromeos/login/login_state.h"
#include "chromeos/settings/timezone_settings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace chromeos {

namespace {

class SetTimeMessageHandler : public content::WebUIMessageHandler,
                              public chromeos::SystemClockClient::Observer,
                              public system::TimezoneSettings::Observer {
 public:
  SetTimeMessageHandler() {
    system::TimezoneSettings::GetInstance()->AddObserver(this);
    chromeos::DBusThreadManager::Get()->GetSystemClockClient()->AddObserver(
        this);
  }

  ~SetTimeMessageHandler() override {
    system::TimezoneSettings::GetInstance()->RemoveObserver(this);
    chromeos::DBusThreadManager::Get()->GetSystemClockClient()->RemoveObserver(
        this);
  }

  // WebUIMessageHandler:
  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "setTimeInSeconds",
        base::BindRepeating(&SetTimeMessageHandler::OnSetTime,
                            base::Unretained(this)));
    web_ui()->RegisterMessageCallback(
        "setTimezone",
        base::BindRepeating(&SetTimeMessageHandler::OnSetTimezone,
                            base::Unretained(this)));
  }

 private:
  // system::SystemClockClient::Observer:
  void SystemClockUpdated() override {
    web_ui()->CallJavascriptFunctionUnsafe("settime.TimeSetter.updateTime");
  }

  // UI actually shows real device timezone, but only allows changing the user
  // timezone. If user timezone settings are different from system, this means
  // that user settings are overriden and must be disabled. (And we will still
  // show the actual device timezone.)
  // system::TimezoneSettings::Observer:
  void TimezoneChanged(const icu::TimeZone& timezone) override {
    base::Value timezone_id(system::TimezoneSettings::GetTimezoneID(timezone));
    web_ui()->CallJavascriptFunctionUnsafe("settime.TimeSetter.setTimezone",
                                           timezone_id);
  }

  // Handler for Javascript call to set the system clock when the user sets a
  // new time. Expects the time as the number of seconds since the Unix
  // epoch, treated as a double.
  void OnSetTime(const base::ListValue* args) {
    double seconds;
    if (!args->GetDouble(0, &seconds)) {
      NOTREACHED();
      return;
    }

    chromeos::DBusThreadManager::Get()->GetSystemClockClient()->SetTime(
        static_cast<int64_t>(seconds));
  }

  // Handler for Javascript call to change the system time zone when the user
  // selects a new time zone. Expects the time zone ID as a string, as it
  // appears in the time zone option values.
  void OnSetTimezone(const base::ListValue* args) {
    std::string timezone_id;
    if (!args->GetString(0, &timezone_id)) {
      NOTREACHED();
      return;
    }

    Profile* profile = Profile::FromBrowserContext(
        web_ui()->GetWebContents()->GetBrowserContext());
    DCHECK(profile);
    system::SetTimezoneFromUI(profile, timezone_id);
  }

  DISALLOW_COPY_AND_ASSIGN(SetTimeMessageHandler);
};

}  // namespace

SetTimeUI::SetTimeUI(content::WebUI* web_ui) : WebDialogUI(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<SetTimeMessageHandler>());

  // Set up the chrome://set-time source.
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUISetTimeHost);

  source->AddLocalizedString("setTimeTitle", IDS_SET_TIME_TITLE);
  source->AddLocalizedString("prompt", IDS_SET_TIME_PROMPT);
  source->AddLocalizedString("doneButton", IDS_SET_TIME_BUTTON_CLOSE);
  source->AddLocalizedString("timezone",
                             IDS_OPTIONS_SETTINGS_TIMEZONE_DESCRIPTION);
  source->AddLocalizedString("dateLabel", IDS_SET_TIME_DATE_LABEL);
  source->AddLocalizedString("timeLabel", IDS_SET_TIME_TIME_LABEL);

  base::DictionaryValue values;
  values.Set("timezoneList", chromeos::system::GetTimezoneList());

  // If we are not logged in, we need to show the time zone dropdown.
  // Otherwise, we can leave |currentTimezoneId| blank.
  std::string current_timezone_id;
  if (!LoginState::Get()->IsUserLoggedIn())
    CrosSettings::Get()->GetString(kSystemTimezone, &current_timezone_id);
  values.SetString("currentTimezoneId", current_timezone_id);
  values.SetDouble("buildTime", base::GetBuildTime().ToJsTime());

  source->AddLocalizedStrings(values);
  source->SetJsonPath("strings.js");

  source->AddResourcePath("set_time.css", IDR_SET_TIME_CSS);
  source->AddResourcePath("set_time.js", IDR_SET_TIME_JS);
  source->SetDefaultResource(IDR_SET_TIME_HTML);

  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui), source);
}

SetTimeUI::~SetTimeUI() {
}

}  // namespace chromeos
