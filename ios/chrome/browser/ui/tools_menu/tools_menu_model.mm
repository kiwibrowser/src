// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/tools_menu/tools_menu_model.h"

#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/experimental_flags.h"
#include "ios/chrome/browser/ui/commands/application_commands.h"
#include "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/tools_menu/new_tab_menu_view_item.h"
#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/tools_menu/reading_list_menu_view_item.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#import "ios/public/provider/chrome/browser/user_feedback/user_feedback_provider.h"
#include "ios/web/public/user_agent.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Menu items can be marked as visible or not when Incognito is enabled.
// The following bits are used for |visibility| field in |MenuItemInfo|.
const NSInteger kVisibleIncognitoOnly = 1 << 0;
const NSInteger kVisibleNotIncognitoOnly = 1 << 1;

// Declare all the possible items. If adding or removing items update
// kToolsMenuNumberOfItems with the new total count.

const MenuItemInfo itemInfoList[kToolsMenuNumberOfItems] = {
    // clang-format off
  { IDS_IOS_TOOLS_MENU_NEW_TAB,           kToolsMenuNewTabId,
    TOOLS_NEW_TAB_ITEM, nullptr,          ToolbarTypeAll,
    0,                                    [NewTabMenuViewItem class] },
  { IDS_IOS_TOOLS_MENU_NEW_INCOGNITO_TAB, kToolsMenuNewIncognitoTabId,
    TOOLS_NEW_INCOGNITO_TAB_ITEM, nullptr,ToolbarTypeAll,
    0,                                    [NewIncognitoTabMenuViewItem class] },
  { IDS_IOS_TOOLS_MENU_CLOSE_ALL_TABS,    kToolsMenuCloseAllTabsId,
    TOOLS_CLOSE_ALL_TABS,
    @selector(closeAllTabs), ToolbarTypeSwitcheriPhone,
    kVisibleNotIncognitoOnly,             nil },
  { IDS_IOS_TOOLS_MENU_CLOSE_ALL_INCOGNITO_TABS,
    kToolsMenuCloseAllIncognitoTabsId,
    TOOLS_CLOSE_ALL_INCOGNITO_TABS,
    @selector(closeAllIncognitoTabs), ToolbarTypeSwitcheriPhone,
    kVisibleIncognitoOnly,                nil },
  { IDS_IOS_TOOLS_MENU_BOOKMARKS,         kToolsMenuBookmarksId,
    TOOLS_SHOW_BOOKMARKS,
    @selector(showBookmarksManager),      ToolbarTypeWebAll,
    0,                                    nil },
  { IDS_IOS_TOOLS_MENU_READING_LIST,      kToolsMenuReadingListId,
    TOOLS_READING_LIST,
    @selector(showReadingList),           ToolbarTypeWebAll,
    0,                                    [ReadingListMenuViewItem class] },
  { IDS_IOS_TOOLS_MENU_RECENT_TABS,       kToolsMenuOtherDevicesId,
    TOOLS_SHOW_RECENT_TABS,
    @selector(showRecentTabs),            ToolbarTypeWebAll,
    kVisibleNotIncognitoOnly,             nil },
  { IDS_HISTORY_SHOW_HISTORY,             kToolsMenuHistoryId,
    TOOLS_SHOW_HISTORY,
    @selector(showHistory),               ToolbarTypeWebAll,
    0,                                    nil },
  { IDS_IOS_OPTIONS_REPORT_AN_ISSUE,      kToolsMenuReportAnIssueId,
    TOOLS_REPORT_AN_ISSUE,
    nullptr,                              ToolbarTypeAll,
    0,                                    nil },
  { IDS_IOS_TOOLS_MENU_FIND_IN_PAGE,      kToolsMenuFindInPageId,
    TOOLS_SHOW_FIND_IN_PAGE,
    @selector(showFindInPage),            ToolbarTypeWebAll,
    0,                                    nil },
  { IDS_IOS_TOOLS_MENU_REQUEST_DESKTOP_SITE,
    kToolsMenuRequestDesktopId,
    TOOLS_REQUEST_DESKTOP_SITE,
    @selector(requestDesktopSite),    ToolbarTypeWebAll,
    0,                                    nil },
  { IDS_IOS_TOOLS_MENU_REQUEST_MOBILE_SITE,
    kToolsMenuRequestMobileId,
    TOOLS_REQUEST_MOBILE_SITE,
    @selector(requestMobileSite),     ToolbarTypeWebAll,
    0,                                    nil },
  { IDS_IOS_TOOLS_MENU_SETTINGS,          kToolsMenuSettingsId,
    TOOLS_SETTINGS_ITEM,
    nullptr,                              ToolbarTypeAll,
    0,                                    nil },
  { IDS_IOS_TOOLS_MENU_HELP_MOBILE,       kToolsMenuHelpId,
    TOOLS_SHOW_HELP_PAGE,
    @selector(showHelpPage),              ToolbarTypeWebAll,
    0,                                    nil },
    // clang-format on
};

bool ToolsMenuItemShouldBeVisible(const MenuItemInfo& item,
                                  ToolbarType toolbarType,
                                  ToolsMenuConfiguration* configuration) {
  if (!(item.toolbar_types & toolbarType))
    return false;

  if (configuration.inIncognito && (item.visibility & kVisibleNotIncognitoOnly))
    return false;

  if (!configuration.inIncognito && (item.visibility & kVisibleIncognitoOnly))
    return false;

  switch (item.title_id) {
    case IDS_IOS_TOOLBAR_SHOW_TABS:
      return IsIPadIdiom();
    case IDS_IOS_OPTIONS_REPORT_AN_ISSUE:
      return ios::GetChromeBrowserProvider()
          ->GetUserFeedbackProvider()
          ->IsUserFeedbackEnabled();
    case IDS_IOS_TOOLS_MENU_REQUEST_DESKTOP_SITE:
      return (configuration.userAgentType != web::UserAgentType::DESKTOP);
    case IDS_IOS_TOOLS_MENU_REQUEST_MOBILE_SITE:
      return (configuration.userAgentType == web::UserAgentType::DESKTOP);
    default:
      return true;
  }
}
