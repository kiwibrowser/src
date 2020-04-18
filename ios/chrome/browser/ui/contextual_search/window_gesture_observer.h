// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_WINDOW_GESTURE_OBSERVER_H_
#define IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_WINDOW_GESTURE_OBSERVER_H_

#import <UIKit/UIGestureRecognizerSubclass.h>
#import <UIKit/UIKit.h>

// A specialized gesture recognizer to be used to detect any gesture in the app,
// excluding those whose touches are solely on a particular view.
// A WindowGestureObserver should be added to the app's main window.
// Use the normal -initWithTarget:action: method to create instances of this
// class.
// This recognizer *never* succeeds and doesn't use the target and action in
// the normal way. Instead, as soon as there is a touch event that is not
// inside |viewToExclude|, this recognizer will call the given action method on
// the target. Then this recognizer will immediately fail and pass the touches
// on to other recognizers.
// Before the action method is called, |touchedView| will be populated with
// a pointer to the view that triggered the call.
// In case there are multiple touches that simultaneously trigger this
// recognizer, only the first touch that is outside of |viewToExclude| will
// trigger a call to the action method. There is no guarantee about how "first"
// will be determined in this case.
// The target and action supplied in the -init will never be called in the
// usual way other regognizers call their action methods, and adding new
// target/action pairs will silently no-op.
// If |viewToExclude| is defined, this recognizer will require all gesture
// recognizers on it present at initialization time to fail before receiving
// touch events.

@interface WindowGestureObserver : UIGestureRecognizer

@property(nonatomic, weak) UIView* viewToExclude;
@property(nonatomic, weak, readonly) UIView* touchedView;
@end

#endif  // IOS_CHROME_BROWSER_UI_CONTEXTUAL_SEARCH_WINDOW_GESTURE_OBSERVER_H_
