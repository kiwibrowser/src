// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_bar_item.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation NewTabPageBarItem
@synthesize title = title_;
@synthesize identifier = identifier_;
@synthesize image = image_;
@synthesize view = view_;

+ (NewTabPageBarItem*)newTabPageBarItemWithTitle:(NSString*)title
                                      identifier:
                                          (ntp_home::PanelIdentifier)identifier
                                           image:(UIImage*)image {
  NewTabPageBarItem* item = [[NewTabPageBarItem alloc] init];
  if (item) {
    item.title = title;
    item.identifier = identifier;
    item.image = image;
  }
  return item;
}

@end
