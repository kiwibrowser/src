// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_COORDINATOR_H_

#import <UIKit/UIKit.h>

namespace ios {
class ChromeBrowserState;
}
class WebOmniboxEditController;
@class CommandDispatcher;
@class OmniboxPopupCoordinator;
@class OmniboxTextFieldIOS;
@protocol OmniboxPopupPositioner;

// The coordinator for the omnibox.
@interface OmniboxCoordinator : NSObject
// The edit controller interfacing the |textField| and the omnibox components
// code. Needs to be set before the coordinator is started.
@property(nonatomic, assign) WebOmniboxEditController* editController;
// Weak reference to ChromeBrowserState;
@property(nonatomic, assign) ios::ChromeBrowserState* browserState;
// The dispatcher for this view controller.
@property(nonatomic, weak) CommandDispatcher* dispatcher;

// The view controller managed by this coordinator. The parent of this
// coordinator is expected to add it to the responder chain.
- (UIViewController*)managedViewController;

// Start this coordinator. When it starts, it expects to have |textField| and
// |editController|.
- (void)start;
// Stop this coordinator.
- (void)stop;
// Indicates if the omnibox is the first responder.
- (BOOL)isOmniboxFirstResponder;
// Inserts text to the omnibox without triggering autocomplete.
// Use this method to insert target URL or search terms for alternative input
// methods, such as QR code scanner or voice search.
- (void)insertTextToOmnibox:(NSString*)string;
// Update the contents and the styling of the omnibox.
- (void)updateOmniboxState;
// Use this method to make the omnibox first responder.
- (void)focusOmnibox;
// Marks the next omnibox focus event source as the search button.
- (void)setNextFocusSourceAsSearchButton;
// Use this method to resign |textField| as the first responder.
- (void)endEditing;
// Creates a child popup coordinator. The popup coordinator is linked to the
// |textField| through components code.
- (OmniboxPopupCoordinator*)createPopupCoordinator:
    (id<OmniboxPopupPositioner>)positioner;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_COORDINATOR_H_
