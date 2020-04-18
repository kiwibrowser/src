// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_LEGACY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_LEGACY_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

@class CommandDispatcher;
@protocol PageInfoPresentation;
@class TabModel;
@protocol UrlLoader;

// Notification sent when the page info is shown.
extern NSString* const kPageInfoWillShowNotification;
// Notification sent when the page info is hidden.
extern NSString* const kPageInfoWillHideNotification;

// The coordinator that manages the display of the Page Info UI. When
// |dispatcher| is set, this coordinator uses |dispatcher| to register itself
// as the target for PageInfoCommmands. These commands can then trigger the
// showing/hiding of the Page Info UI.
@interface PageInfoLegacyCoordinator : ChromeCoordinator

// The dispatcher for this coordinator. When |dispatcher| is set, the
// coordinator will register itself as the target for PageInfoCommands.
@property(nonatomic, weak) CommandDispatcher* dispatcher;

// The UrlLoader to be used by this coordinator.
@property(nonatomic, weak) id<UrlLoader> loader;

// |presentationProvider| provides information and runs tasks necessary to
// present Page Info.
@property(nonatomic, weak) id<PageInfoPresentation> presentationProvider;

// The active TabModel to be used to display Page Info.
@property(nonatomic, weak) TabModel* tabModel;

// Stops listening for dispatcher calls and clears this coordinator's weak
// references. The Page Info UI must not be displayed when this method is
// called.
- (void)disconnect;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_LEGACY_COORDINATOR_H_
