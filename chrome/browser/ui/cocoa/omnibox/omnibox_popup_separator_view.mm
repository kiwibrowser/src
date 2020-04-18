// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/omnibox/omnibox_popup_separator_view.h"

#import "base/mac/scoped_nsobject.h"
#include "chrome/grit/theme_resources.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/resource/resource_bundle.h"

@implementation OmniboxPopupTopSeparatorView

+ (CGFloat)preferredHeight {
  return 1;
}

- (void)drawRect:(NSRect)rect {
  NSRect separatorRect = [self bounds];
  separatorRect.size.height = [self cr_lineWidth];
  [[self strokeColor] set];
  NSRectFillUsingOperation(separatorRect, NSCompositeSourceOver);
}

@end

@implementation OmniboxPopupBottomSeparatorView

+ (CGFloat)preferredHeight {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  NSImage* shadowImage =
      rb.GetNativeImageNamed(IDR_OVERLAY_DROP_SHADOW).ToNSImage();
  return [shadowImage size].height;
}

- (instancetype)initWithFrame:(NSRect)frame forDarkTheme:(BOOL)isDarkTheme {
  if ((self = [self initWithFrame:frame])) {
    isDarkTheme_ = isDarkTheme;
    // For dark themes the OmniboxPopupBottomSeparatorView will render a shadow
    // rather than blit a bitmap. Shadows are expensive to draw so use a layer
    // to cache the result.
    if (isDarkTheme_) {
      [self setWantsLayer:YES];
    }
  }
  return self;
}

- (void)drawRect:(NSRect)rect {
  NSRect bounds = [self bounds];

  if (isDarkTheme_) {
    // There's an image for the shadow the Omnibox casts, but this shadow is
    // an opaque mix of white and black, which makes it look strange against a
    // dark NTP page. For dark mode, draw the shadow in code instead so that
    // it has some transparency.
    base::scoped_nsobject<NSShadow> shadow([[NSShadow alloc] init]);
    [shadow setShadowBlurRadius:8];
    [shadow setShadowColor:[NSColor blackColor]];
    [shadow set];

    // Fill a rect that's out of view to get just the shadow it casts.
    [[NSColor blackColor] set];
    NSRectFill(NSMakeRect(-3, NSMaxY(bounds), NSWidth(bounds) + 6, 5));
    return;
  }

  // Draw the shadow.
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  NSImage* shadowImage =
      rb.GetNativeImageNamed(IDR_OVERLAY_DROP_SHADOW).ToNSImage();
  [shadowImage drawInRect:bounds
                 fromRect:NSZeroRect
                operation:NSCompositeSourceOver
                 fraction:1.0];
}

@end
