// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/profiles/avatar_icon_controller.h"

#include <stddef.h>

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

@interface AvatarIconController (Private)
- (void)setButtonEnabled:(BOOL)flag;
- (NSImage*)compositeImageWithShadow:(NSImage*)image;
- (void)updateAvatarButtonAndLayoutParent:(BOOL)layoutParent;
@end

@implementation AvatarIconController

- (id)initWithBrowser:(Browser*)browser {
  if ((self = [super initWithBrowser:browser])) {
    browser_ = browser;

    base::scoped_nsobject<NSView> container(
        [[NSView alloc] initWithFrame:NSMakeRect(
            0, 0, profiles::kAvatarIconWidth, profiles::kAvatarIconHeight)]);
    [container setWantsLayer:YES];
    [self setView:container];

    NSRect frameRect = NSMakeRect(5, 5, profiles::kAvatarIconWidth,
        profiles::kAvatarIconHeight);
    button_.reset([[NSButton alloc] initWithFrame:frameRect]);
    NSButtonCell* cell = [button_ cell];
    [button_ setButtonType:NSMomentaryLightButton];

    [button_ setImagePosition:NSImageOnly];
    [cell setImageScaling:NSImageScaleProportionallyDown];
    [cell setImagePosition:NSImageBelow];

    // AppKit sets a title for some reason when using |-setImagePosition:|.
    [button_ setTitle:nil];

    [cell setImageDimsWhenDisabled:NO];
    [cell setHighlightsBy:NSContentsCellMask];
    [cell setShowsStateBy:NSContentsCellMask];

    [button_ setBordered:NO];
    [button_ setTarget:self];
    [button_ setAction:@selector(buttonClicked:)];

    [cell accessibilitySetOverrideValue:NSAccessibilityButtonRole
                           forAttribute:NSAccessibilityRoleAttribute];
    [cell accessibilitySetOverrideValue:
        NSAccessibilityRoleDescription(NSAccessibilityButtonRole, nil)
          forAttribute:NSAccessibilityRoleDescriptionAttribute];
    [cell accessibilitySetOverrideValue:
        l10n_util::GetNSString(IDS_PROFILES_BUBBLE_ACCESSIBLE_NAME)
                           forAttribute:NSAccessibilityTitleAttribute];
    [cell accessibilitySetOverrideValue:
        l10n_util::GetNSString(IDS_PROFILES_BUBBLE_ACCESSIBLE_DESCRIPTION)
                           forAttribute:NSAccessibilityHelpAttribute];
    [cell accessibilitySetOverrideValue:
        l10n_util::GetNSString(IDS_PROFILES_BUBBLE_ACCESSIBLE_DESCRIPTION)
                           forAttribute:NSAccessibilityDescriptionAttribute];

    NSImage* icon = NSImageFromImageSkia(
        gfx::CreateVectorIcon(kIncognitoIcon, 24, SK_ColorWHITE));
    [button_ setImage:icon];
    [button_ setEnabled:NO];

    [[self view] addSubview:button_];
  }
  return self;
}

// This will take in an original image and redraw it with a shadow.
- (NSImage*)compositeImageWithShadow:(NSImage*)image {
  gfx::ScopedNSGraphicsContextSaveGState scopedGState;

  base::scoped_nsobject<NSImage> destination(
      [[NSImage alloc] initWithSize:[image size]]);

  NSRect destRect = NSZeroRect;
  destRect.size = [destination size];

  [destination lockFocus];

  base::scoped_nsobject<NSShadow> shadow([[NSShadow alloc] init]);
  [shadow.get() setShadowColor:[NSColor colorWithCalibratedWhite:0.0
                                                           alpha:0.75]];
  [shadow.get() setShadowOffset:NSZeroSize];
  [shadow.get() setShadowBlurRadius:3.0];
  [shadow.get() set];

  [image drawInRect:destRect
           fromRect:NSZeroRect
          operation:NSCompositeSourceOver
           fraction:1.0
     respectFlipped:YES
              hints:nil];

  [destination unlockFocus];

  return destination.autorelease();
}

- (void)updateAvatarButtonAndLayoutParent:(BOOL)layoutParent {}

@end
