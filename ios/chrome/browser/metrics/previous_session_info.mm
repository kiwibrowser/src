// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/metrics/previous_session_info.h"

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "components/version_info/version_info.h"
#import "ios/chrome/browser/metrics/previous_session_info_private.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Key in the NSUserDefaults for a string value that stores the version of the
// last session.
NSString* const kLastRanVersion = @"LastRanVersion";

// Key in the NSUserDefaults for a string value that stores the language of the
// last session.
NSString* const kLastRanLanguage = @"LastRanLanguage";

}  // namespace

namespace previous_session_info_constants {
NSString* const kDidSeeMemoryWarningShortlyBeforeTerminating =
    @"DidSeeMemoryWarning";
}  // namespace previous_session_info_constants

@interface PreviousSessionInfo ()

// Whether beginRecordingCurrentSession was called.
@property(nonatomic, assign) BOOL didBeginRecordingCurrentSession;

// Redefined to be read-write.
@property(nonatomic, assign) BOOL didSeeMemoryWarningShortlyBeforeTerminating;
@property(nonatomic, assign) BOOL isFirstSessionAfterUpgrade;
@property(nonatomic, assign) BOOL isFirstSessionAfterLanguageChange;

@end

@implementation PreviousSessionInfo

@synthesize didBeginRecordingCurrentSession = _didBeginRecordingCurrentSession;
@synthesize didSeeMemoryWarningShortlyBeforeTerminating =
    _didSeeMemoryWarningShortlyBeforeTerminating;
@synthesize isFirstSessionAfterUpgrade = _isFirstSessionAfterUpgrade;
@synthesize isFirstSessionAfterLanguageChange =
    _isFirstSessionAfterLanguageChange;

// Singleton PreviousSessionInfo.
static PreviousSessionInfo* gSharedInstance = nil;

+ (instancetype)sharedInstance {
  if (!gSharedInstance) {
    gSharedInstance = [[PreviousSessionInfo alloc] init];

    // Load the persisted information.
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    gSharedInstance.didSeeMemoryWarningShortlyBeforeTerminating =
        [defaults boolForKey:previous_session_info_constants::
                                 kDidSeeMemoryWarningShortlyBeforeTerminating];
    NSString* lastRanVersion = [defaults stringForKey:kLastRanVersion];
    NSString* currentVersion =
        base::SysUTF8ToNSString(version_info::GetVersionNumber());
    gSharedInstance.isFirstSessionAfterUpgrade =
        ![lastRanVersion isEqualToString:currentVersion];

    NSString* lastRanLanguage = [defaults stringForKey:kLastRanLanguage];
    NSString* currentLanguage = [[NSLocale preferredLanguages] objectAtIndex:0];
    gSharedInstance.isFirstSessionAfterLanguageChange =
        ![lastRanLanguage isEqualToString:currentLanguage];
  }
  return gSharedInstance;
}

+ (void)resetSharedInstanceForTesting {
  gSharedInstance = nil;
}

- (void)beginRecordingCurrentSession {
  if (self.didBeginRecordingCurrentSession)
    return;
  self.didBeginRecordingCurrentSession = YES;

  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

  // Set the new version.
  NSString* currentVersion =
      base::SysUTF8ToNSString(version_info::GetVersionNumber());
  [defaults setObject:currentVersion forKey:kLastRanVersion];

  // Set the new language.
  NSString* currentLanguage = [[NSLocale preferredLanguages] objectAtIndex:0];
  [defaults setObject:currentLanguage forKey:kLastRanLanguage];

  // Clear the memory warning flag.
  [defaults
      removeObjectForKey:previous_session_info_constants::
                             kDidSeeMemoryWarningShortlyBeforeTerminating];

  // Save critical state information for crash detection.
  [defaults synchronize];
}

- (void)setMemoryWarningFlag {
  if (!self.didBeginRecordingCurrentSession)
    return;

  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  [defaults setBool:YES
             forKey:previous_session_info_constants::
                        kDidSeeMemoryWarningShortlyBeforeTerminating];
  // Save critical state information for crash detection.
  [defaults synchronize];
}

- (void)resetMemoryWarningFlag {
  if (!self.didBeginRecordingCurrentSession)
    return;

  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  [defaults
      removeObjectForKey:previous_session_info_constants::
                             kDidSeeMemoryWarningShortlyBeforeTerminating];
  // Save critical state information for crash detection.
  [defaults synchronize];
}

@end
