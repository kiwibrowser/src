// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_MEDIATOR_H_

#import <UIKit/UIKit.h>

@protocol LocationBarConsumer;
class WebStateList;
class ToolbarModel;

// A mediator object that updates the mediator when the web state changes.
@interface LocationBarMediator : NSObject

- (instancetype)initWithToolbarModel:(ToolbarModel*)toolbarModel
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// The WebStateList that this mediator listens for any changes on the active web
// state.
@property(nonatomic, assign) WebStateList* webStateList;

// The toolbar model used by this mediator to extract the current URL and the
// security state.
@property(nonatomic, assign, readonly) ToolbarModel* toolbarModel;

// The consumer for this object. This can change during the lifetime of this
// object and may be nil.
@property(nonatomic, weak) id<LocationBarConsumer> consumer;

// Stops observing all objects.
- (void)disconnect;

@end

#endif  // IOS_CHROME_BROWSER_UI_LOCATION_BAR_LOCATION_BAR_MEDIATOR_H_
