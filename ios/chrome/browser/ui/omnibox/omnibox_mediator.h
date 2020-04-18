// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_MEDIATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/omnibox/omnibox_left_image_consumer.h"

@protocol OmniboxConsumer;

// A mediator object that updates the omnibox according to the model changes.
@interface OmniboxMediator : NSObject<OmniboxLeftImageConsumer>

// The consumer for this object. This can change during the lifetime of this
// object and may be nil.
@property(nonatomic, weak) id<OmniboxConsumer> consumer;

@end

#endif  // IOS_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_MEDIATOR_H_
