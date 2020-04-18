// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/message_center/cocoa/opaque_views.h"

@implementation MCDropDown
// The view must be opaque to render subpixel antialiasing.
- (BOOL)isOpaque {
  return YES;
}

// The view must also fill its background to render subpixel antialiasing.
- (void)drawRect:(NSRect)dirtyRect {
  [backgroundColor_ set];
  NSRectFill(dirtyRect);
  [super drawRect:dirtyRect];
}

- (NSColor*)backgroundColor {
  return backgroundColor_;
}

- (void)setBackgroundColor:(NSColor*)backgroundColor {
  backgroundColor_.reset([backgroundColor retain]);
}
@end

@implementation MCTextField
- (id)initWithFrame:(NSRect)frameRect backgroundColor:(NSColor*)color {
  self = [self initWithFrame:frameRect];
  if (self) {
    [self setBackgroundColor:color];
    backgroundColor_.reset([color retain]);
  }
  return self;
}

- (id)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self) {
    [self setAutoresizingMask:NSViewMinYMargin];
    [self setBezeled:NO];
    [self setBordered:NO];
    [self setEditable:NO];
    [self setSelectable:NO];
    [self setDrawsBackground:YES];
  }
  return self;
}

// The view must be opaque to render subpixel antialiasing.
- (BOOL)isOpaque {
  return YES;
}

// The view must also fill its background to render subpixel antialiasing.
- (void)drawRect:(NSRect)dirtyRect {
  [backgroundColor_ set];
  NSRectFill(dirtyRect);
  [super drawRect:dirtyRect];
}
@end
