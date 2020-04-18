// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/toolbar/media_router_contextual_menu.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/metrics/user_metrics.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/media/router/event_page_request_manager.h"
#include "chrome/browser/media/router/event_page_request_manager_factory.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/mojo/media_router_mojo_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/toolbar/component_toolbar_actions_factory.h"
#include "chrome/browser/ui/toolbar/media_router_action_controller.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "extensions/common/constants.h"
#include "ui/base/models/menu_model_delegate.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"

// static
std::unique_ptr<MediaRouterContextualMenu>
MediaRouterContextualMenu::CreateForToolbar(Browser* browser) {
  return std::make_unique<MediaRouterContextualMenu>(
      browser, true,
      MediaRouterActionController::IsActionShownByPolicy(browser->profile()));
}

// static
std::unique_ptr<MediaRouterContextualMenu>
MediaRouterContextualMenu::CreateForOverflowMenu(Browser* browser) {
  return std::make_unique<MediaRouterContextualMenu>(
      browser, false,
      MediaRouterActionController::IsActionShownByPolicy(browser->profile()));
}

MediaRouterContextualMenu::MediaRouterContextualMenu(Browser* browser,
                                                     bool is_action_in_toolbar,
                                                     bool shown_by_policy)
    : browser_(browser),
      menu_model_(this),
      is_action_in_toolbar_(is_action_in_toolbar) {
  menu_model_.AddItemWithStringId(IDC_MEDIA_ROUTER_ABOUT,
                                  IDS_MEDIA_ROUTER_ABOUT);
  menu_model_.AddSeparator(ui::NORMAL_SEPARATOR);
  menu_model_.AddItemWithStringId(IDC_MEDIA_ROUTER_LEARN_MORE, IDS_LEARN_MORE);
  menu_model_.AddItemWithStringId(IDC_MEDIA_ROUTER_HELP,
                                  IDS_MEDIA_ROUTER_HELP);
  if (shown_by_policy) {
    menu_model_.AddItemWithStringId(IDC_MEDIA_ROUTER_SHOWN_BY_POLICY,
                                    IDS_MEDIA_ROUTER_SHOWN_BY_POLICY);
    menu_model_.SetIcon(
        menu_model_.GetIndexOfCommandId(IDC_MEDIA_ROUTER_SHOWN_BY_POLICY),
        gfx::Image(gfx::CreateVectorIcon(vector_icons::kBusinessIcon, 16,
                                         gfx::kChromeIconGrey)));
  } else {
    menu_model_.AddCheckItemWithStringId(
        IDC_MEDIA_ROUTER_ALWAYS_SHOW_TOOLBAR_ACTION,
        IDS_MEDIA_ROUTER_ALWAYS_SHOW_TOOLBAR_ACTION);
  }
  menu_model_.AddItemWithStringId(IDC_MEDIA_ROUTER_SHOW_IN_TOOLBAR,
                                  GetChangeVisibilityTextId());

  if (!browser_->profile()->IsOffTheRecord()) {
    menu_model_.AddSeparator(ui::NORMAL_SEPARATOR);
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_CHROMEOS)
    menu_model_.AddItemWithStringId(IDC_MEDIA_ROUTER_MANAGE_DEVICES,
                                    IDS_MEDIA_ROUTER_MANAGE_DEVICES);
#endif
    menu_model_.AddCheckItemWithStringId(
        IDC_MEDIA_ROUTER_CLOUD_SERVICES_TOGGLE,
        IDS_MEDIA_ROUTER_CLOUD_SERVICES_TOGGLE);
    menu_model_.AddItemWithStringId(IDC_MEDIA_ROUTER_REPORT_ISSUE,
                                    IDS_MEDIA_ROUTER_REPORT_ISSUE);
  }
}

MediaRouterContextualMenu::~MediaRouterContextualMenu() {}

bool MediaRouterContextualMenu::GetAlwaysShowActionPref() const {
  return MediaRouterActionController::GetAlwaysShowActionPref(
      browser_->profile());
}

void MediaRouterContextualMenu::SetAlwaysShowActionPref(bool always_show) {
  return MediaRouterActionController::SetAlwaysShowActionPref(
      browser_->profile(), always_show);
}

bool MediaRouterContextualMenu::IsCommandIdChecked(int command_id) const {
  if (command_id == IDC_MEDIA_ROUTER_CLOUD_SERVICES_TOGGLE) {
    return browser_->profile()->GetPrefs()->GetBoolean(
        prefs::kMediaRouterEnableCloudServices);
  }
  if (command_id == IDC_MEDIA_ROUTER_ALWAYS_SHOW_TOOLBAR_ACTION)
    return GetAlwaysShowActionPref();
  return false;
}

bool MediaRouterContextualMenu::IsCommandIdEnabled(int command_id) const {
  // If the action is in the ephemeral state, disable the menu item for moving
  // it between the toolbar and the overflow menu, since the preference would
  // not persist.
  if (command_id == IDC_MEDIA_ROUTER_SHOW_IN_TOOLBAR)
    return GetAlwaysShowActionPref();
  return command_id != IDC_MEDIA_ROUTER_SHOWN_BY_POLICY;
}

bool MediaRouterContextualMenu::IsCommandIdVisible(int command_id) const {
  if (command_id == IDC_MEDIA_ROUTER_CLOUD_SERVICES_TOGGLE) {
    // Cloud services preference is not set or used if the user is not signed
    // in.
    SigninManagerBase* signin_manager =
        SigninManagerFactory::GetForProfile(browser_->profile());
    return signin_manager && signin_manager->IsAuthenticated();
  }
  return true;
}

void MediaRouterContextualMenu::ExecuteCommand(int command_id,
                                               int event_flags) {
  const char kAboutPageUrl[] =
      "https://www.google.com/chrome/devices/chromecast/";
  const char kCastHelpCenterPageUrl[] =
      "https://support.google.com/chromecast/topic/3447927";
  const char kCastLearnMorePageUrl[] =
      "https://support.google.com/chromecast/answer/2998338";

  PrefService* pref_service;
  switch (command_id) {
    case IDC_MEDIA_ROUTER_ABOUT:
      ShowSingletonTab(browser_, GURL(kAboutPageUrl));
      break;
    case IDC_MEDIA_ROUTER_ALWAYS_SHOW_TOOLBAR_ACTION:
      SetAlwaysShowActionPref(!GetAlwaysShowActionPref());
      break;
    case IDC_MEDIA_ROUTER_CLOUD_SERVICES_TOGGLE:
      pref_service = browser_->profile()->GetPrefs();
      pref_service->SetBoolean(prefs::kMediaRouterEnableCloudServices,
          !pref_service->GetBoolean(prefs::kMediaRouterEnableCloudServices));

      // If this has been set before, this is a no-op.
      pref_service->SetBoolean(prefs::kMediaRouterCloudServicesPrefSet, true);
      break;
    case IDC_MEDIA_ROUTER_HELP:
      ShowSingletonTab(browser_, GURL(kCastHelpCenterPageUrl));
      base::RecordAction(base::UserMetricsAction(
          "MediaRouter_Ui_Navigate_Help"));
      break;
    case IDC_MEDIA_ROUTER_LEARN_MORE:
      ShowSingletonTab(browser_, GURL(kCastLearnMorePageUrl));
      break;
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_CHROMEOS)
    case IDC_MEDIA_ROUTER_MANAGE_DEVICES:
      ShowSingletonTab(browser_, GURL(chrome::kChromeUICastURL));
      break;
#endif
    case IDC_MEDIA_ROUTER_REPORT_ISSUE:
      ReportIssue();
      break;
    case IDC_MEDIA_ROUTER_SHOW_IN_TOOLBAR:
      ToolbarActionsModel::Get(browser_->profile())
          ->SetActionVisibility(
              ComponentToolbarActionsFactory::kMediaRouterActionId,
              !is_action_in_toolbar_);
      break;
    default:
      NOTREACHED();
  }
}

void MediaRouterContextualMenu::ReportIssue() {
  // Opens feedback page loaded from the media router extension.
  // This is temporary until feedback UI is redesigned.
  media_router::EventPageRequestManager* request_manager =
      media_router::EventPageRequestManagerFactory::GetApiForBrowserContext(
          browser_->profile());
  if (request_manager->media_route_provider_extension_id().empty())
    return;
  std::string feedback_url(
      extensions::kExtensionScheme +
      std::string(url::kStandardSchemeSeparator) +
      request_manager->media_route_provider_extension_id() + "/feedback.html");
  ShowSingletonTab(browser_, GURL(feedback_url));
}

int MediaRouterContextualMenu::GetChangeVisibilityTextId() {
  return is_action_in_toolbar_ ? IDS_EXTENSIONS_HIDE_BUTTON_IN_MENU
                               : IDS_EXTENSIONS_SHOW_BUTTON_IN_TOOLBAR;
}
