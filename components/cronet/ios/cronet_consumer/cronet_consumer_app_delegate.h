// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CRONET_CRONET_CONSUMER_CRONET_CONSUMER_APP_DELEGATE_H_
#define IOS_CRONET_CRONET_CONSUMER_CRONET_CONSUMER_APP_DELEGATE_H_

#import <UIKit/UIKit.h>

@class CronetConsumerViewController;

// The main app controller and UIApplicationDelegate.
@interface CronetConsumerAppDelegate : UIResponder<UIApplicationDelegate>

@property(strong, nonatomic) UIWindow* window;
@property(strong, nonatomic) CronetConsumerViewController* viewController;

@end

#endif  // IOS_CRONET_CRONET_CONSUMER_CRONET_CONSUMER_APP_DELEGATE_H_
