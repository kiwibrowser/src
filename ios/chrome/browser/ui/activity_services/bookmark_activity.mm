// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/activity_services/bookmark_activity.h"

#include "base/logging.h"
#include "ios/chrome/browser/ui/commands/browser_commands.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

NSString* const kBookmarkActivityType = @"com.google.chrome.bookmarkActivity";

}  // namespace

@interface BookmarkActivity ()
// Whether or not the page is bookmarked.
@property(nonatomic, assign) BOOL bookmarked;
// The URL for the activity.
@property(nonatomic, assign) GURL URL;
// The dispatcher that handles when the activity is performed.
@property(nonatomic, weak) id<BrowserCommands> dispatcher;
@end

@implementation BookmarkActivity

@synthesize bookmarked = _bookmarked;
@synthesize dispatcher = _dispatcher;
@synthesize URL = _URL;

- (instancetype)initWithURL:(const GURL&)URL
                 bookmarked:(BOOL)bookmarked
                 dispatcher:(id<BrowserCommands>)dispatcher {
  self = [super init];
  if (self) {
    _URL = URL;
    _bookmarked = bookmarked;
    _dispatcher = dispatcher;
  }
  return self;
}

+ (NSString*)activityIdentifier {
  return kBookmarkActivityType;
}

#pragma mark - UIActivity

- (NSString*)activityType {
  return [BookmarkActivity activityIdentifier];
}

- (NSString*)activityTitle {
  if (self.bookmarked)
    return l10n_util::GetNSString(IDS_IOS_TOOLS_MENU_EDIT_BOOKMARK);
  return l10n_util::GetNSString(IDS_IOS_TOOLS_MENU_ADD_TO_BOOKMARKS);
}

- (UIImage*)activityImage {
  if (self.bookmarked)
    return [UIImage imageNamed:@"activity_services_edit_bookmark"];
  return [UIImage imageNamed:@"activity_services_add_bookmark"];
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
  [self.dispatcher bookmarkPage];
  [self activityDidFinish:YES];
}

@end
