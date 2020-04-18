// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Tools menu presentation notifications.
NSString* const kToolsMenuWillShowNotification =
    @"kToolsMenuWillShowNotification";
NSString* const kToolsMenuWillHideNotification =
    @"kToolsMenuWillHideNotification";
NSString* const kToolsMenuDidShowNotification =
    @"kToolsMenuDidShowNotification";
NSString* const kToolsMenuDidHideNotification =
    @"kToolsMenuDidHideNotification";

// Tools menu item IDs.
NSString* const kToolsMenuReload = @"kToolsMenuReload";
NSString* const kToolsMenuStop = @"kToolsMenuStop";
NSString* const kToolsMenuNewTabId = @"kToolsMenuNewTabId";
NSString* const kToolsMenuNewIncognitoTabId = @"kToolsMenuNewIncognitoTabId";
NSString* const kToolsMenuCloseAllTabsId = @"kToolsMenuCloseAllTabsId";
NSString* const kToolsMenuCloseAllIncognitoTabsId =
    @"kToolsMenuCloseAllIncognitoTabsId";
NSString* const kToolsMenuCloseTabId = @"kToolsMenuCloseTabId";
NSString* const kToolsMenuBookmarksId = @"kToolsMenuBookmarksId";
NSString* const kToolsMenuReadingListId = @"kToolsMenuReadingListId";
NSString* const kToolsMenuOtherDevicesId = @"kToolsMenuOtherDevicesId";
NSString* const kToolsMenuHistoryId = @"kToolsMenuHistoryId";
NSString* const kToolsMenuReportAnIssueId = @"kToolsMenuReportAnIssueId";
NSString* const kToolsMenuFindInPageId = @"kToolsMenuFindInPageId";
NSString* const kToolsMenuRequestDesktopId = @"kToolsMenuRequestDesktopId";
NSString* const kToolsMenuSettingsId = @"kToolsMenuSettingsId";
NSString* const kToolsMenuHelpId = @"kToolsMenuHelpId";
NSString* const kToolsMenuRequestMobileId = @"kToolsMenuRequestMobileId";
NSString* const kToolsMenuReadLater = @"kToolsMenuReadLater";
NSString* const kToolsMenuAddToBookmarks = @"kToolsMenuAddToBookmarks";
NSString* const kToolsMenuEditBookmark = @"kToolsMenuEditBookmark";
NSString* const kToolsMenuSiteInformation = @"kToolsMenuSiteInformation";
NSString* const kToolsMenuPasteAndGo = @"kToolsMenuPasteAndGo";
NSString* const kToolsMenuVoiceSearch = @"kToolsMenuVoiceSearch";
NSString* const kToolsMenuQRCodeSearch = @"kToolsMenuQRCodeSearch";
