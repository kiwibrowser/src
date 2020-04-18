// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/webui/password_manager_internals_ui_ios.h"

#include "base/hash.h"
#include "components/grit/components_resources.h"
#include "components/password_manager/core/browser/password_manager_internals_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/passwords/password_manager_internals_service_factory.h"
#include "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_ui_ios_data_source.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "net/base/escape.h"

using password_manager::PasswordManagerInternalsService;

namespace {

web::WebUIIOSDataSource* CreatePasswordManagerInternalsHTMLSource() {
  web::WebUIIOSDataSource* source =
      web::WebUIIOSDataSource::Create(kChromeUIPasswordManagerInternalsHost);

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

}  //  namespace

PasswordManagerInternalsUIIOS::PasswordManagerInternalsUIIOS(
    web::WebUIIOS* web_ui)
    : WebUIIOSController(web_ui) {
  web_ui->GetWebState()->AddObserver(this);
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromWebUIIOS(web_ui);
  DCHECK(browser_state);
  web::WebUIIOSDataSource::Add(browser_state,
                               CreatePasswordManagerInternalsHTMLSource());
}

PasswordManagerInternalsUIIOS::~PasswordManagerInternalsUIIOS() {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromWebUIIOS(web_ui());
  DCHECK(browser_state);
  if (registered_with_logging_service_) {
    registered_with_logging_service_ = false;
    PasswordManagerInternalsService* service =
        ios::PasswordManagerInternalsServiceFactory::GetForBrowserState(
            browser_state);
    if (service)
      service->UnregisterReceiver(this);
  }
  web_ui()->GetWebState()->RemoveObserver(this);
}

void PasswordManagerInternalsUIIOS::LogSavePasswordProgress(
    const std::string& text) {
  if (!registered_with_logging_service_ || text.empty())
    return;
  std::string no_quotes(text);
  std::replace(no_quotes.begin(), no_quotes.end(), '"', ' ');
  base::Value text_string_value(net::EscapeForHTML(no_quotes));
  web_ui()->CallJavascriptFunction("addSavePasswordProgressLog",
                                   text_string_value);
}

void PasswordManagerInternalsUIIOS::PageLoaded(
    web::WebState* web_state,
    web::PageLoadCompletionStatus load_completion_status) {
  if (load_completion_status != web::PageLoadCompletionStatus::SUCCESS)
    return;

  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromWebUIIOS(web_ui());
  PasswordManagerInternalsService* service =
      ios::PasswordManagerInternalsServiceFactory::GetForBrowserState(
          browser_state);
  // No service means the WebUI is displayed in Incognito.
  base::Value is_incognito(!service);
  web_ui()->CallJavascriptFunction("notifyAboutIncognito", is_incognito);

  if (service) {
    registered_with_logging_service_ = true;

    std::string past_logs(service->RegisterReceiver(this));
    LogSavePasswordProgress(past_logs);
  }
}

void PasswordManagerInternalsUIIOS::WebStateDestroyed(
    web::WebState* web_state) {
  web_state->RemoveObserver(this);
}
