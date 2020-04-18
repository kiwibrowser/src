// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/app/main_application_delegate.h"

#import <Foundation/Foundation.h>

#import "base/mac/foundation_util.h"
#import "ios/chrome/app/chrome_overlay_window_testing.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using MainApplicationDelegateTest = PlatformTest;

// Tests that the application does not crash if |applicationDidEnterBackground|
// is called when the application is launched in background.
// http://crbug.com/437307
TEST_F(MainApplicationDelegateTest, CrashIfNotInitialized) {
  // Save both ChromeBrowserProvider as MainController register new instance.
  ios::ChromeBrowserProvider* stashed_chrome_browser_provider =
      ios::GetChromeBrowserProvider();

  id application = [OCMockObject niceMockForClass:[UIApplication class]];
  UIApplicationState backgroundState = UIApplicationStateBackground;
  [[[application stub] andReturnValue:OCMOCK_VALUE(backgroundState)]
      applicationState];

  MainApplicationDelegate* delegate = [[MainApplicationDelegate alloc] init];
  [delegate application:application didFinishLaunchingWithOptions:nil];
  [delegate applicationDidEnterBackground:application];

  // Clean up the size class recorder, which is created by the main window via
  // a previous call to |application:didFinishLaunchingWithOptions:|, to prevent
  // it from interfering with subsequent tests.
  ChromeOverlayWindow* mainWindow =
      base::mac::ObjCCastStrict<ChromeOverlayWindow>([delegate window]);
  [mainWindow unsetSizeClassRecorder];

  // Restore both ChromeBrowserProvider to its original value and destroy
  // instances created by MainController.
  DCHECK_NE(ios::GetChromeBrowserProvider(), stashed_chrome_browser_provider);
  delete ios::GetChromeBrowserProvider();
  ios::SetChromeBrowserProvider(stashed_chrome_browser_provider);
}
