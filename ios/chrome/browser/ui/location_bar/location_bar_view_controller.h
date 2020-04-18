// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/fullscreen/fullscreen_ui_element.h"
#import "ios/chrome/browser/ui/location_bar/location_bar_consumer.h"

@class OmniboxTextFieldIOS;
@protocol ActivityServiceCommands;
@protocol BrowserCommands;
@protocol ApplicationCommands;

@protocol LocationBarViewControllerDelegate<NSObject>

// Notifies the delegate about a tap on the steady-state location bar.
- (void)locationBarSteadyViewTapped;

@end

// The view controller displaying the location bar. Manages the two states of
// the omnibox - the editing and the non-editing states. In the editing state,
// the omnibox textfield is displayed; in the non-editing state, the current
// location is displayed.
@interface LocationBarViewController : UIViewController<FullscreenUIElement>

- (instancetype)initWithFrame:(CGRect)frame
                         font:(UIFont*)font
                    textColor:(UIColor*)textColor
                    tintColor:(UIColor*)tintColor;

// Sets the edit view to use in the editing state. This must be set before the
// view of this view controller is initialized. This must only be called once.
- (void)setEditView:(UIView*)editView;

@property(nonatomic, assign) BOOL incognito;

// The dispatcher for the share button action.
@property(nonatomic, weak)
    id<ActivityServiceCommands, BrowserCommands, ApplicationCommands>
        dispatcher;

// Delegate for this location bar view controller.
@property(nonatomic, weak) id<LocationBarViewControllerDelegate> delegate;

// Switches between the two states of the location bar:
// - editing state, with the textfield;
// - non-editing state, with location icon and text.
- (void)switchToEditing:(BOOL)editing;

// Updates the location icon to become |icon|.
- (void)updateLocationIcon:(UIImage*)icon;
// Updates the location text in the non-editing mode.
- (void)updateLocationText:(NSString*)text;

// Displays the voice search button instead of the share button in steady state,
// and adds the voice search button to the empty textfield.
@property(nonatomic, assign) BOOL voiceSearchEnabled;

@end

#endif  // IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_VIEW_CONTROLLER_H_
