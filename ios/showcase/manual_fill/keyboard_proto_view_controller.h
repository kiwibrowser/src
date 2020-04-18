// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_SHOWCASE_MANUAL_FILL_KEYBOARD_PROTO_VIEW_CONTROLLER_H_
#define IOS_SHOWCASE_MANUAL_FILL_KEYBOARD_PROTO_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>

namespace manualfill {

// Activates constraints to keep both views in the same place, with the same
// size.
//
// @param sourceView The view to be constraint to destinationView.
// @param destinationView The view to constraint sourceView.
void AddSameConstraints(UIView* sourceView, UIView* destinationView);

// Searches for the first responder in the passed view hierarchy.
//
// @param view The view where the search is going to be done.
// @return The first responder or `nil` if it wasn't found.
UIView* GetFirstResponderSubview(UIView* view);

}  // namespace manualfill

// Protocol to pass any user choice in a picker to be filled.
@protocol ManualFillContentDelegate<NSObject>

// Called after the user manually selects an element to be used as the input.
//
// @param content The string that is interesting to the user in the current
//                context.
- (void)userDidPickContent:(NSString*)content;

@end

// Main class to show the prototype. It contains the code needed to create and
// interact with a WKWebView. Meant to be subclassed.
@interface KeyboardProtoViewController
    : UIViewController<ManualFillContentDelegate>

// The web view to test the prototype.
@property(nonatomic, readonly) WKWebView* webView;

// The last known first responder.
@property(nonatomic) UIView* lastFirstResponder;

// Asynchronously updates the activeFieldID to the current active element.
// Must be called before the web view resigns first responder.
- (void)updateActiveFieldID;

// Tries to inject the passed string into the web view last focused field.
//
// @param string The content to be injected.
- (void)fillLastSelectedFieldWithString:(NSString*)string;

// Calls JS `focus()` on the last active element in an attemp to highlight it.
- (void)callFocusOnLastActiveField;

@end

#endif  // IOS_SHOWCASE_MANUAL_FILL_KEYBOARD_PROTO_VIEW_CONTROLLER_H_
