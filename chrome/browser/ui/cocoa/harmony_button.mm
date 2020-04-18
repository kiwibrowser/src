// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/harmony_button.h"

#import "base/mac/scoped_cftyperef.h"
#import "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/nsview_additions.h"
#import "ui/base/theme_provider.h"

namespace {

NSColor* GetBackgroundColor(HoverState state, BOOL dark_theme) {
  if (dark_theme) {
    switch (state) {
      case kHoverStateNone:
        return [NSColor colorWithCalibratedWhite:1 alpha:0.12];
      case kHoverStateMouseOver:
        return [NSColor colorWithCalibratedWhite:1 alpha:0.18];
      case kHoverStateMouseDown:
        return [NSColor colorWithCalibratedWhite:1 alpha:0.22];
    }
  } else {
    switch (state) {
      case kHoverStateNone:
        return [NSColor colorWithCalibratedWhite:1 alpha:1];
      case kHoverStateMouseOver:
        return [NSColor colorWithCalibratedWhite:0.98 alpha:1];
      case kHoverStateMouseDown:
        return [NSColor colorWithCalibratedWhite:0.95 alpha:1];
    }
  }
}

NSColor* GetBorderColor(BOOL dark_theme) {
  return [NSColor colorWithCalibratedWhite:dark_theme ? 1 : 0 alpha:0.3];
}

NSColor* GetShadowColor() {
  return NSColor.blackColor;
}

constexpr CGFloat kFontSize = 12;

constexpr CGFloat kTextAlpha = 0x8A / (CGFloat)0xFF;

constexpr CGSize kNormalShadowOffset{0, 0};
constexpr CGSize kMouseOverShadowOffset{0, 1};

constexpr CGFloat kNormalShadowOpacity = 0;
constexpr CGFloat kMouseOverShadowOpacity = 0.1;

constexpr CGFloat kNormalShadowRadius = 0;
constexpr CGFloat kMouseOverShadowRadius = 2;

constexpr CGFloat kCornerRadius = 3;
constexpr CGFloat kXPadding = 16;
constexpr CGFloat kMinWidth = 64;
constexpr CGFloat kHeight = 28;

// The text is a bit too low by default; offset the title rect to center it.
constexpr CGFloat kTextYOffset = 1;

constexpr NSTimeInterval kTransitionDuration = 0.25;

}  // namespace

@interface HarmonyButtonCell : NSButtonCell
@end

@implementation HarmonyButtonCell
- (NSRect)titleRectForBounds:(NSRect)rect {
  rect.origin.y -= kTextYOffset;
  return rect;
}
@end

@implementation HarmonyButton

+ (instancetype)buttonWithTitle:(NSString*)title
                         target:(id)target
                         action:(SEL)action {
  HarmonyButton* button = [[[self alloc] initWithFrame:NSZeroRect] autorelease];
  button.title = title;
  button.target = target;
  button.action = action;
  [button sizeToFit];
  return button;
}

+ (Class)cellClass {
  return [HarmonyButtonCell class];
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect])) {
    self.bezelStyle = NSRoundedBezelStyle;
    self.bordered = NO;
    self.wantsLayer = YES;
    CALayer* layer = self.layer;
    layer.shadowColor = GetShadowColor().CGColor;
    layer.cornerRadius = kCornerRadius;
    [self updateBorderWidth];
    [self updateHoverButtonAppearanceAnimated:NO];
  }
  return self;
}

- (void)updateBorderWidth {
  self.layer.borderWidth = [self cr_lineWidth];
}

- (void)updateHoverButtonAppearanceAnimated:(BOOL)animated {
  if (!self.window)
    return;

  const ui::ThemeProvider* themeProvider = [self.window themeProvider];
  const BOOL darkTheme = [self.window hasDarkTheme];
  CALayer* layer = self.layer;

  if (animated) {
    [NSAnimationContext beginGrouping];
    NSAnimationContext* context = [NSAnimationContext currentContext];
    context.allowsImplicitAnimation = YES;
    context.duration = kTransitionDuration;
  } else {
    [layer removeAllAnimations];
  }

  switch (self.hoverState) {
    case kHoverStateNone:
      layer.shadowOffset = kNormalShadowOffset;
      layer.shadowOpacity = kNormalShadowOpacity;
      layer.shadowRadius = kNormalShadowRadius;
      break;
    case kHoverStateMouseOver:
    case kHoverStateMouseDown:
      layer.shadowOffset = kMouseOverShadowOffset;
      layer.shadowOpacity = kMouseOverShadowOpacity;
      layer.shadowRadius = kMouseOverShadowRadius;
      break;
  }

  layer.borderColor =
      (themeProvider && themeProvider->ShouldIncreaseContrast())
          ? CGColorGetConstantColor(darkTheme ? kCGColorWhite : kCGColorBlack)
          : GetBorderColor(darkTheme).CGColor;
  layer.backgroundColor =
      GetBackgroundColor(self.hoverState, darkTheme).CGColor;

  if (animated) {
    [NSAnimationContext endGrouping];
  }
}

// HoverButton overrides.

- (void)setHoverState:(HoverState)state {
  if (state == hoverState_) {
    return;
  }
  const BOOL animated =
      state != kHoverStateMouseDown && hoverState_ != kHoverStateMouseDown;
  hoverState_ = state;
  [self updateHoverButtonAppearanceAnimated:animated];
}

// NSButton overrides.

- (void)setTitle:(NSString*)title {
  NSColor* textColor;
  if (const ui::ThemeProvider* themeProvider = [self.window themeProvider]) {
    textColor = themeProvider->GetNSColor(ThemeProperties::COLOR_TAB_TEXT);
    if (!themeProvider->ShouldIncreaseContrast())
      textColor = [textColor colorWithAlphaComponent:kTextAlpha];
  } else {
    textColor = [NSColor controlTextColor];
  }

  NSFont* font;
  if (@available(macOS 10.11, *)) {
    font = [NSFont systemFontOfSize:kFontSize weight:NSFontWeightMedium];
  } else {
    font = [[NSFontManager sharedFontManager]
        convertWeight:YES
               ofFont:[NSFont systemFontOfSize:kFontSize]];
  }

  base::scoped_nsobject<NSMutableParagraphStyle> paragraphStyle(
      [[NSParagraphStyle defaultParagraphStyle] mutableCopy]);
  paragraphStyle.get().alignment = NSCenterTextAlignment;
  base::scoped_nsobject<NSAttributedString> attributedTitle(
      [[NSAttributedString alloc]
          initWithString:title
              attributes:@{
                NSFontAttributeName : font,
                NSForegroundColorAttributeName : textColor,
                NSParagraphStyleAttributeName : paragraphStyle,
              }]);
  if (![self.attributedTitle isEqual:attributedTitle])
    self.attributedTitle = attributedTitle;
}

// NSControl overrides.

- (void)sizeToFit {
  NSSize size = self.attributedTitle.size;
  size.width += kXPadding * 2;
  size = [self backingAlignedRect:NSMakeRect(0, 0, size.width, size.height)
                          options:NSAlignAllEdgesOutward]
             .size;
  [self setFrameSize:NSMakeSize(size.width > kMinWidth ? size.width : kMinWidth,
                                kHeight)];
}

// NSView overrides.

- (void)layout {
  CALayer* layer = self.layer;
  layer.shadowPath =
      base::ScopedCFTypeRef<CGPathRef>(CGPathCreateWithRoundedRect(
          layer.bounds,
          MIN(layer.cornerRadius, CGRectGetWidth(layer.bounds) / 2),
          MIN(layer.cornerRadius, CGRectGetHeight(layer.bounds) / 2), nullptr));
  [self updateHoverButtonAppearanceAnimated:NO];
  self.title = self.title;  // Match the theme.
  [super layout];
}

- (void)drawFocusRingMask {
  CGFloat radius = self.layer.cornerRadius;
  [[NSBezierPath bezierPathWithRoundedRect:self.bounds
                                   xRadius:radius
                                   yRadius:radius] fill];
}

- (void)viewDidChangeBackingProperties {
  [super viewDidChangeBackingProperties];
  [self updateBorderWidth];
}

// This is undocumented. Without it, NSView sets `self.layer.masksToBounds =
// YES` whenever the view is resized, clipping the shadow.
- (BOOL)clipsToBounds {
  return NO;
}

@end
