// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/print_activity.h"

#include "base/logging.h"
#include "ios/chrome/browser/ui/commands/browser_commands.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

NSString* const kPrintActivityType = @"com.google.chrome.printActivity";

}  // namespace

@implementation PrintActivity
@synthesize dispatcher = _dispatcher;

+ (NSString*)activityIdentifier {
  return kPrintActivityType;
}

#pragma mark - UIActivity

- (NSString*)activityType {
  return [PrintActivity activityIdentifier];
}

- (NSString*)activityTitle {
  return l10n_util::GetNSString(IDS_IOS_SHARE_MENU_PRINT_ACTION);
}

- (UIImage*)activityImage {
  return [UIImage imageNamed:@"activity_services_print"];
}

- (BOOL)canPerformWithActivityItems:(NSArray*)activityItems {
  return YES;
}

- (void)prepareWithActivityItems:(NSArray*)activityItems {
}

+ (UIActivityCategory)activityCategory {
  return UIActivityCategoryAction;
}

- (void)performActivity {
  [self.dispatcher printTab];
  [self activityDidFinish:YES];
}

@end
