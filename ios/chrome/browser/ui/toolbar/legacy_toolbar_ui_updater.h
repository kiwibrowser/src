// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_UI_UPDATER_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_UI_UPDATER_H_

#import <UIKit/UIKit.h>

@protocol ToolbarOwner;
@class ToolbarUIState;
class WebStateList;

@protocol ToolbarHeightProviderForFullscreen
// Returns the height of the part of the toolbar that is only displayed when not
// in fullscreen.
- (CGFloat)nonFullscreenToolbarHeight;
@end

// Helper object that uses navigation events to update a ToolbarUIState.
@interface LegacyToolbarUIUpdater : NSObject

// The toolbar UI being updated by this object.
@property(nonatomic, strong, readonly, nonnull) ToolbarUIState* toolbarUI;

// Designated initializer that uses navigation events from |webStateList| and
// the height provided by |toolbarOwner| to update |state|'s broadcast value.
- (nullable instancetype)
initWithToolbarUI:(nonnull ToolbarUIState*)toolbarUI
     toolbarOwner:(nonnull id<ToolbarHeightProviderForFullscreen>)owner
     webStateList:(nonnull WebStateList*)webStateList NS_DESIGNATED_INITIALIZER;
- (nullable instancetype)init NS_UNAVAILABLE;

// Starts updating |state|.
- (void)startUpdating;

// Stops updating |state|.
- (void)stopUpdating;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_UI_UPDATER_H_
