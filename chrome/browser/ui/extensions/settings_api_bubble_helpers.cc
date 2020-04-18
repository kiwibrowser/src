// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/extensions/settings_api_bubble_helpers.h"

#include <utility>

#include "build/build_config.h"
#include "chrome/browser/extensions/ntp_overridden_bubble_delegate.h"
#include "chrome/browser/extensions/settings_api_bubble_delegate.h"
#include "chrome/browser/extensions/settings_api_helpers.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/extensions/extension_message_bubble_bridge.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/common/extensions/manifest_handlers/settings_overrides_handler.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/navigation_entry.h"
#include "extensions/common/constants.h"

namespace extensions {

namespace {

// Whether the NTP bubble is enabled. By default, this is limited to Windows and
// ChromeOS, but can be overridden for testing.
#if defined(OS_WIN) || defined(OS_CHROMEOS)
bool g_ntp_bubble_enabled = true;
#else
bool g_ntp_bubble_enabled = false;
#endif

void ShowSettingsApiBubble(SettingsApiOverrideType type,
                           Browser* browser) {
  ToolbarActionsModel* model = ToolbarActionsModel::Get(browser->profile());
  if (model->has_active_bubble())
    return;

  std::unique_ptr<ExtensionMessageBubbleController> settings_api_bubble(
      new ExtensionMessageBubbleController(
          new SettingsApiBubbleDelegate(browser->profile(), type), browser));
  if (!settings_api_bubble->ShouldShow())
    return;

  settings_api_bubble->SetIsActiveBubble();
  ToolbarActionsBar* toolbar_actions_bar =
      browser->window()->GetToolbarActionsBar();
  std::unique_ptr<ToolbarActionsBarBubbleDelegate> bridge(
      new ExtensionMessageBubbleBridge(std::move(settings_api_bubble)));
  toolbar_actions_bar->ShowToolbarActionBubbleAsync(std::move(bridge));
}

}  // namespace

void SetNtpBubbleEnabledForTesting(bool enabled) {
  g_ntp_bubble_enabled = enabled;
}

void MaybeShowExtensionControlledHomeNotification(Browser* browser) {
#if !defined(OS_WIN) && !defined(OS_MACOSX)
  return;
#endif

  ShowSettingsApiBubble(BUBBLE_TYPE_HOME_PAGE, browser);
}

void MaybeShowExtensionControlledSearchNotification(
    content::WebContents* web_contents,
    AutocompleteMatch::Type match_type) {
#if !defined(OS_WIN) && !defined(OS_MACOSX)
  return;
#endif

  if (AutocompleteMatch::IsSearchType(match_type) &&
      match_type != AutocompleteMatchType::SEARCH_OTHER_ENGINE) {
    Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
    if (browser)
      ShowSettingsApiBubble(BUBBLE_TYPE_SEARCH_ENGINE, browser);
  }
}

void MaybeShowExtensionControlledNewTabPage(
    Browser* browser, content::WebContents* web_contents) {
  if (!g_ntp_bubble_enabled)
    return;

  // Acknowledge existing extensions if necessary.
  NtpOverriddenBubbleDelegate::MaybeAcknowledgeExistingNtpExtensions(
      browser->profile());

  content::NavigationEntry* entry =
      web_contents->GetController().GetActiveEntry();
  if (!entry)
    return;
  GURL active_url = entry->GetURL();
  if (!active_url.SchemeIs(extensions::kExtensionScheme))
    return;  // Not a URL that we care about.

  // See if the current active URL matches a transformed NewTab URL.
  GURL ntp_url(chrome::kChromeUINewTabURL);
  bool ignored_param;
  content::BrowserURLHandler::GetInstance()->RewriteURLIfNecessary(
      &ntp_url,
      web_contents->GetBrowserContext(),
      &ignored_param);
  if (ntp_url != active_url)
    return;  // Not being overridden by an extension.

  ToolbarActionsModel* model = ToolbarActionsModel::Get(browser->profile());
  if (model->has_active_bubble())
    return;

  std::unique_ptr<ExtensionMessageBubbleController> ntp_overridden_bubble(
      new ExtensionMessageBubbleController(
          new NtpOverriddenBubbleDelegate(browser->profile()), browser));
  if (!ntp_overridden_bubble->ShouldShow())
    return;

  ntp_overridden_bubble->SetIsActiveBubble();
  ToolbarActionsBar* toolbar_actions_bar =
      browser->window()->GetToolbarActionsBar();
  std::unique_ptr<ToolbarActionsBarBubbleDelegate> bridge(
      new ExtensionMessageBubbleBridge(std::move(ntp_overridden_bubble)));
  toolbar_actions_bar->ShowToolbarActionBubbleAsync(std::move(bridge));
}

}  // namespace extensions
