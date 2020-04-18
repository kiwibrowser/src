// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/download/google_drive_app_util.h"

#import <UIKit/UIKit.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kGoogleDriveITunesItemIdentifier = @"507874739";
NSString* const kGoogleDriveAppURLScheme = @"googledrive";
NSString* const kGoogleDriveAppBundleID = @"com.google.Drive";

NSURL* GetGoogleDriveAppUrl() {
  NSURLComponents* google_drive_url = [[NSURLComponents alloc] init];
  google_drive_url.scheme = kGoogleDriveAppURLScheme;
  return google_drive_url.URL;
}

bool IsGoogleDriveAppInstalled() {
  return [[UIApplication sharedApplication] canOpenURL:GetGoogleDriveAppUrl()];
}
