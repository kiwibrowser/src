// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_APP_APPLICATION_DELEGATE_APP_STATE_H_
#define IOS_CHROME_APP_APPLICATION_DELEGATE_APP_STATE_H_

#import <UIKit/UIKit.h>

@protocol AppNavigation;
@protocol BrowserLauncher;
@protocol BrowserViewInformation;
@protocol TabOpening;
@protocol TabSwitching;
@protocol StartupInformation;
@class DeviceSharingManager;
@class MainApplicationDelegate;
@class MemoryWarningHelper;
@class MetricsMediator;
@class TabModel;

// Represents the application state and responds to application state changes
// and system events.
@interface AppState : NSObject

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)
initWithBrowserLauncher:(id<BrowserLauncher>)browserLauncher
     startupInformation:(id<StartupInformation>)startupInformation
    applicationDelegate:(MainApplicationDelegate*)applicationDelegate
    NS_DESIGNATED_INITIALIZER;

// YES if the user has ever interacted with the application. May be NO if the
// application has been woken up by the system for background work.
@property(nonatomic, readonly) BOOL userInteracted;

// Window for the application, it is not set during the initialization method.
// Set the property before calling methods related to it.
@property(nonatomic, weak) UIWindow* window;

// Saves the launchOptions to be used from -newTabFromLaunchOptions. If the
// application is in background, initialize the browser to basic. If not, launch
// the browser.
// Returns whether additional delegate handling should be performed (call to
// -performActionForShortcutItem or -openURL by the system for example)
- (BOOL)requiresHandlingAfterLaunchWithOptions:(NSDictionary*)launchOptions
                               stateBackground:(BOOL)stateBackground;

// Whether the application is in Safe Mode.
- (BOOL)isInSafeMode;

// Logs duration of the session in the main tab model and records that chrome is
// no longer in cold start.
- (void)willResignActiveTabModel;

// Called when the application is getting terminated. It stops all outgoing
// requests, config updates, clears the device sharing manager and stops the
// mainChrome instance.
- (void)applicationWillTerminate:(UIApplication*)application
           applicationNavigation:(id<AppNavigation>)appNavigation;

// Resumes the session: reinitializing metrics and opening new tab if necessary.
// User sessions are defined in terms of BecomeActive/ResignActive so that
// session boundaries include things like turning the screen off or getting a
// phone call, not just switching apps.
- (void)resumeSessionWithTabOpener:(id<TabOpening>)tabOpener
                       tabSwitcher:(id<TabSwitching>)tabSwitcher;

// Called when going into the background. iOS already broadcasts, so
// stakeholders can register for it directly.
- (void)applicationDidEnterBackground:(UIApplication*)application
                         memoryHelper:(MemoryWarningHelper*)memoryHelper
                  tabSwitcherIsActive:(BOOL)tabSwitcherIsActive;

// Called when returning to the foreground. Resets and uploads the metrics.
// Starts the browser to foreground if needed.
- (void)applicationWillEnterForeground:(UIApplication*)application
                       metricsMediator:(MetricsMediator*)metricsMediator
                          memoryHelper:(MemoryWarningHelper*)memoryHelper
                             tabOpener:(id<TabOpening>)tabOpener
                         appNavigation:(id<AppNavigation>)appNavigation;

// Sets the return value for -didFinishLaunchingWithOptions that determines if
// UIKit should make followup delegate calls such as
// -performActionForShortcutItem or -openURL.
- (void)launchFromURLHandled:(BOOL)URLHandled;

@end

#endif  // IOS_CHROME_APP_APPLICATION_DELEGATE_APP_STATE_H_
