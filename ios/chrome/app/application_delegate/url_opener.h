// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_APP_APPLICATION_DELEGATE_URL_OPENER_H_
#define IOS_CHROME_APP_APPLICATION_DELEGATE_URL_OPENER_H_

#import <UIKit/UIKit.h>

@class AppState;
@protocol StartupInformation;
@protocol TabOpening;

// Handles the URL-opening methods of the ApplicationDelegate. This class has
// only class methods and should not be instantiated.
@interface URLOpener : NSObject

- (instancetype)init NS_UNAVAILABLE;

// Handles open URL. The registered URL Schemes are defined in project
// variables ${CHROMIUM_URL_SCHEME_x}.
// The url can either be empty, in which case the app is simply opened or
// can contain an URL that will be opened in a new tab.
// Returns YES if the url can be opened, NO otherwise.
+ (BOOL)openURL:(NSURL*)url
     applicationActive:(BOOL)applicationActive
               options:(NSDictionary<NSString*, id>*)options
             tabOpener:(id<TabOpening>)tabOpener
    startupInformation:(id<StartupInformation>)startupInformation;

// Handles launch options: converts them to open URL options and opens them.
+ (void)handleLaunchOptions:(NSDictionary*)launchOptions
          applicationActive:(BOOL)applicationActive
                  tabOpener:(id<TabOpening>)tabOpener
         startupInformation:(id<StartupInformation>)startupInformation
                   appState:(AppState*)appState;
@end

#endif  // IOS_CHROME_APP_APPLICATION_DELEGATE_URL_OPENER_H_
