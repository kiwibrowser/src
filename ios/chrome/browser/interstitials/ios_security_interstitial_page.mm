// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/interstitials/ios_security_interstitial_page.h"

#include "base/logging.h"
#include "components/grit/components_resources.h"
#include "components/prefs/pref_service.h"
#include "components/security_interstitials/core/common_string_util.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#include "ios/web/public/interstitials/web_interstitial.h"
#include "ios/web/public/web_state/web_state.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

IOSSecurityInterstitialPage::IOSSecurityInterstitialPage(
    web::WebState* web_state,
    const GURL& request_url)
    : web_state_(web_state),
      request_url_(request_url),
      web_interstitial_(nullptr) {
  // Creating web_interstitial_ without showing it leaks memory, so don't
  // create it here.
}

IOSSecurityInterstitialPage::~IOSSecurityInterstitialPage() {}

void IOSSecurityInterstitialPage::Show() {
  DCHECK(!web_interstitial_);
  web_interstitial_ = web::WebInterstitial::CreateHtmlInterstitial(
      web_state_, ShouldCreateNewNavigation(), request_url_,
      std::unique_ptr<web::HtmlWebInterstitialDelegate>(this));
  web_interstitial_->Show();
  AfterShow();
}

std::string IOSSecurityInterstitialPage::GetHtmlContents() const {
  base::DictionaryValue load_time_data;
  PopulateInterstitialStrings(&load_time_data);
  webui::SetLoadTimeDataDefaults(
      GetApplicationContext()->GetApplicationLocale(), &load_time_data);
  std::string html = ui::ResourceBundle::GetSharedInstance()
                         .GetRawDataResource(IDR_SECURITY_INTERSTITIAL_HTML)
                         .as_string();
  webui::AppendWebUiCssTextDefaults(&html);
  return webui::GetI18nTemplateHtml(html, &load_time_data);
}

base::string16 IOSSecurityInterstitialPage::GetFormattedHostName() const {
  return security_interstitials::common_string_util::GetFormattedHostName(
      request_url_);
}

bool IOSSecurityInterstitialPage::IsPrefEnabled(const char* pref_name) const {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromBrowserState(web_state_->GetBrowserState());
  return browser_state->GetPrefs()->GetBoolean(pref_name);
}
