// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_PROTECTED_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_PROTECTED_H_

#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller_constants.h"

@interface ToolbarController (Protected)

// The view containing all the content of the toolbar. It respects the trailing
// and leading anchors of the safe area.
@property(nonatomic, readonly, strong) UIView* contentView;

// Called when buttons are pressed. Records action metrics.
// Subclasses must call |super| if they override this method.
- (IBAction)recordUserMetrics:(id)sender;

// Returns the stackButton (a.k.a. tabSwitcherButton). Used by subclasses to
// set display or dismiss target-actions.
- (UIButton*)stackButton;

// Returns the area that subclasses should use for adding their own
// controls.
- (CGRect)specificControlsArea;

// Returns the image enum for a given button object.  If the user taps a
// button before its secondary images have been loaded, the image(s) will be
// loaded then, synchronously.  Called by -standardButtonPressed.
- (int)imageEnumForButton:(UIButton*)button;

// Returns the resource ID for the image with enum |index|, corresponding to
// |style| and |state|.  Returns 0 if there is not a corresponding ID.
// If a subclass extends the enum ToolbarButtonName, it must also override
// this method to provide the correct mapping from enum to resource ID.
- (int)imageIdForImageEnum:(int)index
                     style:(ToolbarControllerStyle)style
                  forState:(ToolbarButtonUIState)state;

// Sets up |button| with images named by the given |imageEnum| and the current
// toolbar style.  Sets images synchronously for |initialState|, and
// asynchronously for the other states. Optionally sets the image for the
// disabled state as well.  Meant to be called during initialization.
// Note:  |withImageEnum| should be one of the ToolbarButtonName values, or an
// extended value provided by a subclass.  It is an int to support
// "subclassing" of the enum and overriding helper functions.
- (void)setUpButton:(UIButton*)button
       withImageEnum:(int)imageEnum
     forInitialState:(UIControlState)initialState
    hasDisabledImage:(BOOL)hasDisabledImage
       synchronously:(BOOL)synchronously;

// TRUE if |imageEnum| should be flipped when in RTL layout.
// Currently none of this class' images have this property, but subclasses
// can override this method if they need to flip some of their images.
- (BOOL)imageShouldFlipForRightToLeftLayoutDirection:(int)imageEnum;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_LEGACY_TOOLBAR_CONTROLLER_PROTECTED_H_
