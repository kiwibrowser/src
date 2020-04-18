// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/inline_login_ui.h"

#include <memory>

#include "base/command_line.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/webui/metrics_handler.h"
#include "chrome/browser/ui/webui/signin/inline_login_handler_impl.h"
#include "chrome/browser/ui/webui/test_files_request_filter.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_switches.h"

namespace {

content::WebUIDataSource* CreateWebUIDataSource() {
  content::WebUIDataSource* source =
        content::WebUIDataSource::Create(chrome::kChromeUIChromeSigninHost);
  source->OverrideContentSecurityPolicyObjectSrc("object-src chrome:;");
  source->SetJsonPath("strings.js");

  source->SetDefaultResource(IDR_INLINE_LOGIN_HTML);

  // Only add a filter when runing as test.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  const bool is_running_test = command_line->HasSwitch(::switches::kTestName) ||
                               command_line->HasSwitch(::switches::kTestType);
  if (is_running_test)
    source->SetRequestFilter(test::GetTestFilesRequestFilter());

  source->AddResourcePath("inline_login.css", IDR_INLINE_LOGIN_CSS);
  source->AddResourcePath("inline_login.js", IDR_INLINE_LOGIN_JS);
  source->AddResourcePath("gaia_auth_host.js", IDR_GAIA_AUTH_AUTHENTICATOR_JS);

  source->AddLocalizedString("title", IDS_CHROME_SIGNIN_TITLE);
  source->AddLocalizedString(
      "accessibleCloseButtonLabel", IDS_SIGNIN_ACCESSIBLE_CLOSE_BUTTON);
  source->AddLocalizedString(
      "accessibleBackButtonLabel", IDS_SIGNIN_ACCESSIBLE_BACK_BUTTON);
  return source;
}

} // empty namespace

InlineLoginUI::InlineLoginUI(content::WebUI* web_ui)
    : WebDialogUI(web_ui),
      auth_extension_(Profile::FromWebUI(web_ui)) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, CreateWebUIDataSource());
  web_ui->AddMessageHandler(std::make_unique<InlineLoginHandlerImpl>());
  web_ui->AddMessageHandler(std::make_unique<MetricsHandler>());

  content::WebContents* contents = web_ui->GetWebContents();
  // Required for intercepting extension function calls when the page is loaded
  // in a bubble (not a full tab, thus tab helpers are not registered
  // automatically).
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      contents);
  extensions::TabHelper::CreateForWebContents(contents);
  // Ensure that the login UI has a tab ID, which will allow the GAIA auth
  // extension's background script to tell it apart from iframes injected by
  // other extensions.
  SessionTabHelper::CreateForWebContents(contents);
}

InlineLoginUI::~InlineLoginUI() {}
