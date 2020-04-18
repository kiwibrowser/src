// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_window.h"

#import "base/logging.h"
#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_constants.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.h"
#include "chrome/grit/generated_resources.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSColor+Luminance.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"

using bookmarks::kBookmarkBarMenuCornerRadius;

namespace {

// Material Design bookmark folder window background white.
const CGFloat kMDFolderWindowBackgroundColor = 237. / 255.;

}  // namespace

@interface BookmarkBarFolderWindow (Accessibility)

- (NSString*)accessibilityTitle;

@end

@implementation BookmarkBarFolderWindow

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)windowStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)deferCreation {
  if ((self = [super initWithContentRect:contentRect
                               styleMask:NSBorderlessWindowMask // override
                                 backing:bufferingType
                                   defer:deferCreation])) {
    [self setBackgroundColor:[NSColor clearColor]];
    [self setOpaque:NO];
  }
  return self;
}

- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return NO;
}

// Override of keyDown as the NSWindow default implementation beeps.
- (void)keyDown:(NSEvent *)theEvent {
}

// If the menu doesn't have a separate accessibleTitle, it will get announced as
// its normal window title, which is "BmbPopUpWindow".
- (NSString*)accessibilityTitle {
  return l10n_util::GetNSString(IDS_ACCNAME_BOOKMARKS_MENU);
}

@end


@implementation BookmarkBarFolderWindowContentView

+ (NSColor*)backgroundColor {
  static NSColor* backgroundColor =
      [[NSColor colorWithGenericGamma22White:kMDFolderWindowBackgroundColor
                                       alpha:1.0] retain];
  return backgroundColor;
}

- (void)drawRect:(NSRect)rect {
  // Like NSMenus, only the bottom corners are rounded.
  NSBezierPath* bezier =
      [NSBezierPath bezierPathWithRoundedRect:[self bounds]
                                      xRadius:kBookmarkBarMenuCornerRadius
                                      yRadius:kBookmarkBarMenuCornerRadius];
  [[BookmarkBarFolderWindowContentView backgroundColor] set];
  [bezier fill];
}

@end


@implementation BookmarkBarFolderWindowScrollView

// We want "draw background" of the NSScrollView in the xib to be NOT
// checked.  That allows us to round the bottom corners of the folder
// window.  However that also allows some scrollWheel: events to leak
// into the NSWindow behind it (even in a different application).
// Better to plug the scroll leak than to round corners for M5.
- (void)scrollWheel:(NSEvent *)theEvent {
  DCHECK([[[self window] windowController]
           respondsToSelector:@selector(scrollWheel:)]);
  [[[self window] windowController] scrollWheel:theEvent];
}

@end
