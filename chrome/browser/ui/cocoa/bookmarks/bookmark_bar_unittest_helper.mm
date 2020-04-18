// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_unittest_helper.h"

@interface NSArray (BookmarkBarUnitTestHelper)

// A helper function for scanning an array of buttons looking for the
// button with the given |title|.
- (BookmarkButton*)buttonWithTitleEqualTo:(NSString*)title;

@end


@implementation NSArray (BookmarkBarUnitTestHelper)

- (BookmarkButton*)buttonWithTitleEqualTo:(NSString*)title {
  for (BookmarkButton* button in self) {
    if ([[button title] isEqualToString:title])
      return button;
  }
  return nil;
}

@end

@implementation BookmarkBarController (BookmarkBarUnitTestHelper)

- (BookmarkButton*)buttonWithTitleEqualTo:(NSString*)title {
  return [[self buttons] buttonWithTitleEqualTo:title];
}

@end

@implementation BookmarkBarFolderController(BookmarkBarUnitTestHelper)

- (BookmarkButton*)buttonWithTitleEqualTo:(NSString*)title {
  return [[self buttons] buttonWithTitleEqualTo:title];
}

@end

@implementation BookmarkButton(BookmarkBarUnitTestHelper)

- (NSPoint)center {
  NSRect frame = [self frame];
  NSPoint center = NSMakePoint(NSMidX(frame), NSMidY(frame));
  center = [[self superview] convertPoint:center toView:nil];
  return center;
}

- (NSPoint)top {
  NSRect frame = [self frame];
  NSPoint top = NSMakePoint(NSMidX(frame), NSMaxY(frame));
  top = [[self superview] convertPoint:top toView:nil];
  return top;
}

- (NSPoint)bottom {
  NSRect frame = [self frame];
  NSPoint bottom = NSMakePoint(NSMidX(frame), NSMinY(frame));
  bottom = [[self superview] convertPoint:bottom toView:nil];
  return bottom;
}

- (NSPoint)left {
  NSRect frame = [self frame];
  NSPoint left = NSMakePoint(NSMinX(frame), NSMidY(frame));
  left = [[self superview] convertPoint:left toView:nil];
  return left;
}

- (NSPoint)right {
  NSRect frame = [self frame];
  NSPoint right = NSMakePoint(NSMaxX(frame), NSMidY(frame));
  right = [[self superview] convertPoint:right toView:nil];
  return right;
}

@end
