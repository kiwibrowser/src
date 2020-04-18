// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_BOOKMARK_ACTIVITY_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_BOOKMARK_ACTIVITY_H_

#import <UIKit/UIKit.h>

@protocol BrowserCommands;
class GURL;

// Activity that adds the page to bookmarks.
@interface BookmarkActivity : UIActivity

// Initialize the bookmark activity with the |URL| to check to know if the page
// is already bookmarked in the |bookmarkModel|. The |dispatcher| is used to add
// the page to the bookmarks.
- (instancetype)initWithURL:(const GURL&)URL
                 bookmarked:(BOOL)bookmarked
                 dispatcher:(id<BrowserCommands>)dispatcher;
- (instancetype)init NS_UNAVAILABLE;

// Identifier for the bookmark activity.
+ (NSString*)activityIdentifier;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_BOOKMARK_ACTIVITY_H_
