// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/l10n_util.h"

#include "base/i18n/rtl.h"
#include "base/mac/availability.h"
#include "base/mac/mac_util.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/common/chrome_features.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMUILocalizerAndLayoutTweaker.h"

namespace cocoa_l10n_util {

NSInteger CompareFrameY(id view1, id view2, void* context) {
  CGFloat y1 = NSMinY([view1 frame]);
  CGFloat y2 = NSMinY([view2 frame]);
  if (y1 < y2)
    return NSOrderedAscending;
  else if (y1 > y2)
    return NSOrderedDescending;
  else
    return NSOrderedSame;
}

NSSize WrapOrSizeToFit(NSView* view) {
  if ([view isKindOfClass:[NSTextField class]]) {
    NSTextField* textField = static_cast<NSTextField*>(view);
    if ([textField isEditable])
      return NSZeroSize;
    CGFloat heightChange =
        [GTMUILocalizerAndLayoutTweaker sizeToFitFixedWidthTextField:textField];
    return NSMakeSize(0.0, heightChange);
  }
  if ([view isKindOfClass:[NSMatrix class]]) {
    NSMatrix* radioGroup = static_cast<NSMatrix*>(view);
    [GTMUILocalizerAndLayoutTweaker wrapRadioGroupForWidth:radioGroup];
    return [GTMUILocalizerAndLayoutTweaker sizeToFitView:view];
  }
  if ([view isKindOfClass:[NSButton class]]) {
    NSButton* button = static_cast<NSButton*>(view);
    NSButtonCell* buttonCell = [button cell];
    // Decide it's a checkbox via showsStateBy and highlightsBy.
    if (([buttonCell showsStateBy] == NSCellState) &&
        ([buttonCell highlightsBy] == NSCellState)) {
      [GTMUILocalizerAndLayoutTweaker wrapButtonTitleForWidth:button];
      return [GTMUILocalizerAndLayoutTweaker sizeToFitView:view];
    }
  }
  return [GTMUILocalizerAndLayoutTweaker sizeToFitView:view];
}

CGFloat VerticallyReflowGroup(NSArray* views) {
  views = [views sortedArrayUsingFunction:CompareFrameY
                                  context:NULL];
  CGFloat localVerticalShift = 0;
  for (NSInteger index = [views count] - 1; index >= 0; --index) {
    NSView* view = [views objectAtIndex:index];

    NSSize delta = WrapOrSizeToFit(view);
    localVerticalShift += delta.height;
    if (localVerticalShift) {
      NSPoint origin = [view frame].origin;
      origin.y -= localVerticalShift;
      [view setFrameOrigin:origin];
    }
  }
  return localVerticalShift;
}

NSString* ReplaceNSStringPlaceholders(NSString* formatString,
                                      const base::string16& a,
                                      size_t* offset) {
  return base::SysUTF16ToNSString(base::ReplaceStringPlaceholders(
      base::SysNSStringToUTF16(formatString), a, offset));
}

NSString* TooltipForURLAndTitle(NSString* url, NSString* title) {
  if ([title length] == 0)
    return url;
  else if ([url length] == 0 || [url isEqualToString:title])
    return title;
  else
    return [NSString stringWithFormat:@"%@\n%@", title, url];
}

bool ShouldDoExperimentalRTLLayout() {
  return base::i18n::IsRTL() && base::FeatureList::IsEnabled(features::kMacRTL);
}

bool ShouldFlipWindowControlsInRTL() {
  return ShouldDoExperimentalRTLLayout() && base::mac::IsAtLeastOS10_12();
}

void ApplyForcedRTL() {
  NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;

  // -registerDefaults: won't do the trick here because these defaults exist
  // (in the global domain) to reflect the system locale. They need to be set
  // in Chrome's domain to supersede the system value.
  switch (base::i18n::GetForcedTextDirection()) {
    case base::i18n::RIGHT_TO_LEFT:
      [defaults setBool:YES forKey:@"AppleTextDirection"];
      [defaults setBool:YES forKey:@"NSForceRightToLeftWritingDirection"];
      break;
    case base::i18n::LEFT_TO_RIGHT:
      [defaults setBool:YES forKey:@"AppleTextDirection"];
      [defaults setBool:NO forKey:@"NSForceRightToLeftWritingDirection"];
      break;
    default:
      [defaults removeObjectForKey:@"AppleTextDirection"];
      [defaults removeObjectForKey:@"NSForceRightToLeftWritingDirection"];
      break;
  }
}

// TODO(lgrey): Remove these when deployment target is 10.12.
#if defined(MAC_OS_X_VERSION_10_12) && \
    (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
#warning LeadingCellImagePosition/TrailingCellImagePosition \
  should be removed since the deployment target is >= 10.12
#endif

NSCellImagePosition LeadingCellImagePosition() {
#if defined(MAC_OS_X_VERSION_10_12) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
  if (@available(macOS 10.12, *)) {
    return NSImageLeading;
  }
#endif
  return ShouldDoExperimentalRTLLayout() ? NSImageRight : NSImageLeft;
}
NSCellImagePosition TrailingCellImagePosition() {
#if defined(MAC_OS_X_VERSION_10_12) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_12
  if (@available(macOS 10.12, *)) {
    return NSImageTrailing;
  }
#endif
  return ShouldDoExperimentalRTLLayout() ? NSImageLeft : NSImageRight;
}

NSRectEdge LeadingEdge() {
  return ShouldDoExperimentalRTLLayout() ? NSMaxXEdge : NSMinXEdge;
}

NSRectEdge TrailingEdge() {
  return ShouldDoExperimentalRTLLayout() ? NSMinXEdge : NSMaxXEdge;
}

// Adapted from Apple's RTL docs (goo.gl/cBaFnT)
NSImage* FlippedImage(NSImage* image) {
  const NSSize size = [image size];
  NSImage* flipped_image = [[[NSImage alloc] initWithSize:size] autorelease];

  [flipped_image lockFocus];
  [[NSGraphicsContext currentContext]
      setImageInterpolation:NSImageInterpolationHigh];

  NSAffineTransform* transform = [NSAffineTransform transform];
  [transform translateXBy:size.width yBy:0];
  [transform scaleXBy:-1 yBy:1];
  [transform concat];

  [image drawAtPoint:NSZeroPoint
            fromRect:NSMakeRect(0, 0, size.width, size.height)
           operation:NSCompositeSourceOver
            fraction:1.0];

  [flipped_image unlockFocus];

  return flipped_image;
}

void FlipAllSubviewsIfNecessary(NSView* view) {
  if (!ShouldDoExperimentalRTLLayout())
    return;
  CGFloat width = NSWidth([view frame]);
  for (NSView* subview in [view subviews]) {
    NSRect subviewFrame = [subview frame];
    subviewFrame.origin.x =
        width - NSWidth(subviewFrame) - NSMinX(subviewFrame);
    [subview setFrame:subviewFrame];
    BOOL hasMinXMargin = subview.autoresizingMask & NSViewMinXMargin;
    BOOL hasMaxXMargin = subview.autoresizingMask & NSViewMaxXMargin;
    if (hasMinXMargin && hasMaxXMargin) {
      // No-op. Skip reversing autoresizing mask if both horizontal margins
      // are flexible.
    } else if (hasMinXMargin) {
      subview.autoresizingMask &= ~NSViewMinXMargin;
      subview.autoresizingMask |= NSViewMaxXMargin;
    } else if (hasMaxXMargin) {
      subview.autoresizingMask &= ~NSViewMaxXMargin;
      subview.autoresizingMask |= NSViewMinXMargin;
    }
  }
}

}  // namespace cocoa_l10n_util
