// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/password_manager_internals/password_manager_internals_ui.h"

#include <algorithm>
#include <set>

#include "base/stl_util.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "components/grit/components_resources.h"
#include "components/password_manager/content/browser/password_manager_internals_service_factory.h"
#include "components/password_manager/core/browser/password_manager_internals_service.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "net/base/escape.h"

using password_manager::PasswordManagerInternalsService;
using password_manager::PasswordManagerInternalsServiceFactory;

namespace {

content::WebUIDataSource* CreatePasswordManagerInternalsHTMLSource() {
  content::WebUIDataSource* source = content::WebUIDataSource::Create(
      chrome::kChromeUIPasswordManagerInternalsHost);

  source->AddResourcePath(
      "password_manager_internals.js",
      IDR_PASSWORD_MANAGER_INTERNALS_PASSWORD_MANAGER_INTERNALS_JS);
  source->AddResourcePath(
      "password_manager_internals.css",
      IDR_PASSWORD_MANAGER_INTERNALS_PASSWORD_MANAGER_INTERNALS_CSS);
  source->SetDefaultResource(
      IDR_PASSWORD_MANAGER_INTERNALS_PASSWORD_MANAGER_INTERNALS_HTML);
  source->UseGzip();
  return source;
}

}  // namespace

PasswordManagerInternalsUI::PasswordManagerInternalsUI(content::WebUI* web_ui)
    : WebUIController(web_ui),
      WebContentsObserver(web_ui->GetWebContents()),
      registered_with_logging_service_(false) {
  // Set up the chrome://password-manager-internals/ source.
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui),
                                CreatePasswordManagerInternalsHTMLSource());
}

PasswordManagerInternalsUI::~PasswordManagerInternalsUI() {
  UnregisterFromLoggingServiceIfNecessary();
}

void PasswordManagerInternalsUI::DidStartLoading() {
  // In case this tab is being reloaded, unregister itself until the reload is
  // completed.
  UnregisterFromLoggingServiceIfNecessary();
}

void PasswordManagerInternalsUI::DidStopLoading() {
  DCHECK(!registered_with_logging_service_);
  PasswordManagerInternalsService* service =
      PasswordManagerInternalsServiceFactory::GetForBrowserContext(
          Profile::FromWebUI(web_ui()));
  // No service means the WebUI is displayed in Incognito.
  base::Value is_incognito(!service);
  web_ui()->CallJavascriptFunctionUnsafe("notifyAboutIncognito", is_incognito);

  if (service) {
    registered_with_logging_service_ = true;

    std::string past_logs(service->RegisterReceiver(this));
    LogSavePasswordProgress(past_logs);
  }

  // Reset the first run experience for auto sign-in if the user opened
  // chrome://password-manager-internals/?reset_fre .
  // TODO(crbug.com/599454) This may be removed in M53, when the credential
  // manager API has been launched to stable.
  GURL url = web_ui()->GetWebContents()->GetVisibleURL();
  if (url.is_valid() && url.has_query()) {
    std::vector<std::string> query_parameters = base::SplitString(
        url.query(), "&", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    if (base::ContainsValue(query_parameters, "reset_fre"))
      ResetAutoSignInFirstRunExperience();
  }
}

void PasswordManagerInternalsUI::LogSavePasswordProgress(
    const std::string& text) {
  if (!registered_with_logging_service_ || text.empty())
    return;
  std::string no_quotes(text);
  std::replace(no_quotes.begin(), no_quotes.end(), '"', ' ');
  base::Value text_string_value(net::EscapeForHTML(no_quotes));
  web_ui()->CallJavascriptFunctionUnsafe("addSavePasswordProgressLog",
                                         text_string_value);
}

void PasswordManagerInternalsUI::UnregisterFromLoggingServiceIfNecessary() {
  if (!registered_with_logging_service_)
    return;
  registered_with_logging_service_ = false;
  PasswordManagerInternalsService* service =
      PasswordManagerInternalsServiceFactory::GetForBrowserContext(
          Profile::FromWebUI(web_ui()));
  if (service)
    service->UnregisterReceiver(this);
}

void PasswordManagerInternalsUI::ResetAutoSignInFirstRunExperience() {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (profile->GetProfileType() == Profile::REGULAR_PROFILE) {
    profile->GetPrefs()->SetBoolean(
        password_manager::prefs::kWasAutoSignInFirstRunExperienceShown, false);

    PasswordManagerInternalsService* service =
      PasswordManagerInternalsServiceFactory::GetForBrowserContext(
          Profile::FromWebUI(web_ui()));
    if (service)
      service->ProcessLog("Reset auto sign-in first run experience: yes");
  }
}
