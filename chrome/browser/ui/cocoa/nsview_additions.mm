// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/nsview_additions.h"

#import "chrome/browser/ui/cocoa/l10n_util.h"

@implementation NSView (ChromeBrowserAdditions)

+ (NSAutoresizingMaskOptions)cr_localizedAutoresizingMask:
    (NSAutoresizingMaskOptions)mask {
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    // If exactly one of NSViewMinXMargin and NSViewMaxXMargin are set…
    if (((mask & NSViewMinXMargin) != 0) != ((mask & NSViewMaxXMargin) != 0)) {
      // …then swap it with the opposite one.
      mask ^= NSViewMinXMargin | NSViewMaxXMargin;
    }
  }
  return mask;
}

- (NSRect)cr_localizedRect:(NSRect)rect {
  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    rect.origin.x = NSWidth(self.bounds) - NSWidth(rect) - NSMinX(rect);
  }
  return rect;
}

@end
