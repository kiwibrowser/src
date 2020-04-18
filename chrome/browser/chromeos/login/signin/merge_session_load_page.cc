// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin/merge_session_load_page.h"

#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/signin/oauth2_login_manager_factory.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_preferences_util.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "net/base/escape.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "ui/base/webui/web_ui_util.h"

using content::BrowserThread;
using content::InterstitialPage;
using content::WebContents;

namespace {

// Delay time for showing interstitial page.
const int kShowDelayTimeMS = 1000;

// Maximum time for showing interstitial page.
const int kTotalWaitTimeMS = 10000;

}  // namespace

namespace chromeos {

MergeSessionLoadPage::MergeSessionLoadPage(
    WebContents* web_contents,
    const GURL& url,
    const merge_session_throttling_utils::CompletionCallback& callback)
    : callback_(callback),
      proceeded_(false),
      web_contents_(web_contents),
      url_(url) {
  interstitial_page_ = InterstitialPage::Create(web_contents, true, url, this);
}

MergeSessionLoadPage::~MergeSessionLoadPage() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void MergeSessionLoadPage::Show() {
  OAuth2LoginManager* manager = GetOAuth2LoginManager();
  if (manager && manager->ShouldBlockTabLoading()) {
    manager->AddObserver(this);
    interstitial_page_->Show();
  } else {
    interstitial_page_->Proceed();
  }
}

std::string MergeSessionLoadPage::GetHTMLContents() {
  base::DictionaryValue strings;
  strings.SetString("title", web_contents_->GetTitle());
  // Set the timeout to show the page.
  strings.SetInteger("show_delay_time", kShowDelayTimeMS);
  strings.SetInteger("total_wait_time", kTotalWaitTimeMS);
  // TODO(zelidrag): Flip the message to IDS_MERGE_SESSION_LOAD_HEADLINE
  // after merge.
  strings.SetString("heading",
                    l10n_util::GetStringUTF16(IDS_TAB_LOADING_TITLE));

  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  webui::SetLoadTimeDataDefaults(app_locale, &strings);

  base::StringPiece html(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_MERGE_SESSION_LOAD_HTML));
  return webui::GetI18nTemplateHtml(html, &strings);
}

void MergeSessionLoadPage::OverrideRendererPrefs(
    content::RendererPreferences* prefs) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents_->GetBrowserContext());
  renderer_preferences_util::UpdateFromSystemSettings(prefs, profile,
                                                      web_contents_);
}

void MergeSessionLoadPage::OnProceed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  proceeded_ = true;
  NotifyBlockingPageComplete();
}

void MergeSessionLoadPage::OnDontProceed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Ignore if it's already proceeded.
  if (proceeded_)
    return;
  NotifyBlockingPageComplete();
}

void MergeSessionLoadPage::CommandReceived(const std::string& cmd) {
  std::string command(cmd);
  // The Jasonified response has quotes, remove them.
  if (command.length() > 1 && command[0] == '"')
    command = command.substr(1, command.length() - 2);

  if (command == "proceed") {
    interstitial_page_->Proceed();
  } else {
    DVLOG(1) << "Unknown command:" << cmd;
  }
}

void MergeSessionLoadPage::NotifyBlockingPageComplete() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  OAuth2LoginManager* manager = GetOAuth2LoginManager();
  if (manager)
    manager->RemoveObserver(this);

  // Tell the MergeSessionNavigationThrottle to resume the navigation on the UI
  // thread.
  if (!callback_.is_null())
    callback_.Run();
}

OAuth2LoginManager* MergeSessionLoadPage::GetOAuth2LoginManager() {
  content::BrowserContext* browser_context = web_contents_->GetBrowserContext();
  if (!browser_context)
    return NULL;

  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (!profile)
    return NULL;

  return OAuth2LoginManagerFactory::GetInstance()->GetForProfile(profile);
}

void MergeSessionLoadPage::OnSessionRestoreStateChanged(
    Profile* user_profile,
    OAuth2LoginManager::SessionRestoreState state) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  OAuth2LoginManager* manager = GetOAuth2LoginManager();
  DVLOG(1) << "Merge session should "
           << (!manager->ShouldBlockTabLoading() ? " NOT " : "")
           << " be blocking now, " << state;
  if (!manager->ShouldBlockTabLoading()) {
    manager->RemoveObserver(this);
    interstitial_page_->Proceed();
  }
}

}  // namespace chromeos
