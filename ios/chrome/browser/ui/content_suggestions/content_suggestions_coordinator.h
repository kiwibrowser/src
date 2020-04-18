// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_panel_protocol.h"

namespace ios {
class ChromeBrowserState;
}

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class ContentSuggestionsHeaderViewController;
@protocol NewTabPageControllerDelegate;
@protocol OmniboxFocuser;
@protocol FakeboxFocuser;
@protocol SnackbarCommands;
@protocol UrlLoader;
class WebStateList;

// Coordinator to manage the Suggestions UI via a
// ContentSuggestionsViewController.
@interface ContentSuggestionsCoordinator
    : ChromeCoordinator<NewTabPagePanelProtocol>

// BrowserState used to create the ContentSuggestionFactory.
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;
// URLLoader used to open pages.
@property(nonatomic, weak) id<UrlLoader> URLLoader;
@property(nonatomic, assign) WebStateList* webStateList;
@property(nonatomic, weak) id<NewTabPageControllerDelegate> toolbarDelegate;
@property(nonatomic, weak) id<ApplicationCommands,
                              BrowserCommands,
                              OmniboxFocuser,
                              FakeboxFocuser,
                              SnackbarCommands,
                              UrlLoader>
    dispatcher;
// Whether the Suggestions UI is displayed. If this is true, start is a no-op.
@property(nonatomic, readonly) BOOL visible;

@property(nonatomic, strong, readonly)
    ContentSuggestionsHeaderViewController* headerController;

@property(nonatomic, strong, readonly)
    UICollectionViewController* viewController;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_COORDINATOR_H_
