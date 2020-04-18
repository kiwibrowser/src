// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_CONTROLLER_H_

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_model.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_cell.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@protocol SigninPresenter;
@protocol SyncPresenter;

namespace ios {
class ChromeBrowserState;
}

@class TabSwitcherLocalSessionCell;
@class TabSwitcherPanelController;

@protocol TabSwitcherPanelControllerDelegate<NSObject>

// Called when the user selects the cell representing the distant tab |tab|.
- (void)tabSwitcherPanelController:
            (TabSwitcherPanelController*)tabSwitcherPanelController
               didSelectDistantTab:(synced_sessions::DistantTab*)tab;

// Called when the user selects the cell representing the local tab |tab|.
- (void)tabSwitcherPanelController:
            (TabSwitcherPanelController*)tabSwitcherPanelController
                 didSelectLocalTab:(Tab*)tab;

// Called when the user pressed the close button on the cell representing the
// local tab |tab|.
- (void)tabSwitcherPanelController:
            (TabSwitcherPanelController*)tabSwitcherPanelController
                  didCloseLocalTab:(Tab*)tab;

// Called when the overlay view visibility changed.
- (void)tabSwitcherPanelControllerDidUpdateOverlayViewVisibility:
    (TabSwitcherPanelController*)tabSwitcherPanelController;

@end

@class TabSwitcherPanelView;
// The tab switcher panel controller manages a panel view.
// It can manage either distant or local sessions and uses a TabSwitcherModel
// as its data source.
@class TabSwitcherCache;

@interface TabSwitcherPanelController : NSObject

@property(nonatomic, readonly, weak) TabSwitcherPanelView* view;
@property(nonatomic, weak) id<TabSwitcherPanelControllerDelegate> delegate;
@property(nonatomic, readonly) TabSwitcherSessionType sessionType;
@property(nonatomic, readonly, weak) id<SigninPresenter, SyncPresenter>
    presenter;
@property(nonatomic, readonly, weak) id<ApplicationCommands, BrowserCommands>
    dispatcher;

// Initializes a controller for a view showing local tabs. |offTheRecord|
// determines whether the tabs will be shown for the incognito browser state or
// not. |model| is used to populate the view and must not be nil.
- (instancetype)initWithModel:(TabSwitcherModel*)model
        forLocalSessionOfType:(TabSwitcherSessionType)type
                    withCache:(TabSwitcherCache*)cache
                 browserState:(ios::ChromeBrowserState*)browserState
                    presenter:(id<SigninPresenter, SyncPresenter>)presenter
                   dispatcher:
                       (id<ApplicationCommands, BrowserCommands>)dispatcher;

// Initializes a controller for a view showing the tabs of a distant session.
// |model| is used to populate the view and must not be nil.
- (instancetype)initWithModel:(TabSwitcherModel*)model
     forDistantSessionWithTag:(std::string const&)sessionTag
                 browserState:(ios::ChromeBrowserState*)browserState
                    presenter:(id<SigninPresenter, SyncPresenter>)presenter
                   dispatcher:
                       (id<ApplicationCommands, BrowserCommands>)dispatcher;

// Tells the controller that the collectionview's content may need to be
// updated.
- (void)updateCollectionViewIfNeeded;

// Returns the tag of the distant session.
- (std::string)sessionTag;

// Returns true if the new tab button should be shown.
- (BOOL)shouldShowNewTabButton;

// Scrolls the collection view so that the tab cell at the given index is
// visible.
- (void)scrollTabIndexToVisible:(NSInteger)index triggerLayout:(BOOL)layout;

// Returns the cell for the tab at the given index.
// This methods should only be used for animation purpose.
- (TabSwitcherLocalSessionCell*)localSessionCellForTabAtIndex:(NSInteger)index;

- (CGRect)localSessionCellFrameForTabAtIndex:(NSInteger)index;

// Reloads cells.
- (void)reload;

- (UICollectionView*)collectionView;

// Returns the distantSession owned and displayed by this controller.
- (const synced_sessions::DistantSession*)distantSession;

@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_SWITCHER_TAB_SWITCHER_PANEL_CONTROLLER_H_
