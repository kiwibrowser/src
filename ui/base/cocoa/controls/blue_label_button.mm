// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/controls/blue_label_button.h"

#include "base/mac/foundation_util.h"
#include "skia/ext/skia_utils_mac.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/cocoa/scoped_cg_context_smooth_fonts.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

const CGFloat kCornerRadius = 2;

const CGFloat kTopBottomTextPadding = 7;
const CGFloat kLeftRightTextPadding = 15;
const SkColor kTextShadowColor = SkColorSetRGB(0x53, 0x8c, 0xea);

const SkColor kShadowColor = SkColorSetRGB(0xe9, 0xe9, 0xe9);
const SkColor kDefaultColor = SkColorSetRGB(0x5a, 0x97, 0xff);
const SkColor kHoverColor = SkColorSetRGB(0x55, 0x8f, 0xf3);
const SkColor kPressedColor = SkColorSetRGB(0x42, 0x79, 0xd8);

const SkColor kInnerRingColor = SkColorSetRGB(0x64, 0x9e, 0xff);
const SkColor kFocusInnerRingColor = SkColorSetRGB(0xad, 0xcc, 0xff);
const SkColor kPressInnerRingColor = SkColorSetRGB(0x3f, 0x73, 0xcd);

const SkColor kOuterRingColor = SkColorSetRGB(0x2b, 0x67, 0xce);
const SkColor kPressOuterRingColor = SkColorSetRGB(0x23, 0x52, 0xa2);

const int kFontSizeDelta = ui::ResourceBundle::kSmallFontDelta;

@interface BlueLabelButtonCell : NSButtonCell

+ (NSAttributedString*)generateAttributedString:(NSString*)buttonText;

@end

@implementation BlueLabelButton

+ (Class)cellClass {
  return [BlueLabelButtonCell class];
}

- (id)initWithFrame:(NSRect)frameRect {
  if ((self = [super initWithFrame:frameRect])) {
    [self setBezelStyle:NSSmallSquareBezelStyle];
  }
  return self;
}

@end

@implementation BlueLabelButtonCell

+ (NSAttributedString*)generateAttributedString:(NSString*)buttonText {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  NSFont* buttonFont = rb.GetFontWithDelta(kFontSizeDelta).GetNativeFont();
  base::scoped_nsobject<NSMutableParagraphStyle> buttonTextParagraphStyle(
      [[NSMutableParagraphStyle alloc] init]);
  [buttonTextParagraphStyle setAlignment:NSCenterTextAlignment];

  base::scoped_nsobject<NSShadow> shadow([[NSShadow alloc] init]);
  [shadow setShadowOffset:NSMakeSize(0, -1)];
  [shadow setShadowBlurRadius:0];
  [shadow setShadowColor:skia::SkColorToSRGBNSColor(kTextShadowColor)];

  NSDictionary* buttonTextAttributes = @{
    NSParagraphStyleAttributeName : buttonTextParagraphStyle,
    NSFontAttributeName : buttonFont,
    NSForegroundColorAttributeName : [NSColor whiteColor],
    NSShadowAttributeName : shadow.get()
  };
  base::scoped_nsobject<NSAttributedString> attributedButtonText(
      [[NSAttributedString alloc] initWithString:buttonText
                                      attributes:buttonTextAttributes]);
  return attributedButtonText.autorelease();
}

- (NSSize)cellSize {
  NSAttributedString* attributedTitle =
      [[self class] generateAttributedString:[self title]];
  NSSize textSize = [attributedTitle size];

  // Add 1 to maintain identical height w/ previous drawing code.
  textSize.height += 2 * kTopBottomTextPadding + 1;
  textSize.width += 2 * kLeftRightTextPadding;
  return textSize;
}

- (NSRect)drawTitle:(NSAttributedString*)title
          withFrame:(NSRect)frame
             inView:(NSView*)controlView {
  // Fuzz factor to adjust for the drop shadow. Based on visual inspection.
  frame.origin.y -= 1;

  ui::ScopedCGContextSmoothFonts fontSmoothing;
  NSAttributedString* attributedTitle =
      [[self class] generateAttributedString:[self title]];
  [attributedTitle drawInRect:frame];
  return frame;
}

- (void)drawBezelWithFrame:(NSRect)frame
                    inView:(NSView*)controlView {
  NSColor* centerColor;
  NSColor* innerColor;
  NSColor* outerColor;
  HoverState hoverState =
      [base::mac::ObjCCastStrict<HoverButton>(controlView) hoverState];
  // Leave a sliver of height 1 for the button drop shadow.
  frame.size.height -= 1;

  if (hoverState == kHoverStateMouseDown && [self isHighlighted]) {
    centerColor = skia::SkColorToSRGBNSColor(kPressedColor);
    innerColor = skia::SkColorToSRGBNSColor(kPressInnerRingColor);
    outerColor = skia::SkColorToSRGBNSColor(kPressOuterRingColor);
  } else {
    centerColor = hoverState == kHoverStateMouseOver ?
        skia::SkColorToSRGBNSColor(kHoverColor) :
        skia::SkColorToSRGBNSColor(kDefaultColor);
    innerColor = [self showsFirstResponder] ?
        skia::SkColorToSRGBNSColor(kFocusInnerRingColor) :
        skia::SkColorToSRGBNSColor(kInnerRingColor);
    outerColor = skia::SkColorToSRGBNSColor(kOuterRingColor);
  }
  {
    gfx::ScopedNSGraphicsContextSaveGState context;
    base::scoped_nsobject<NSShadow> shadow([[NSShadow alloc] init]);
    [shadow setShadowOffset:NSMakeSize(0, -1)];
    [shadow setShadowBlurRadius:1.0];
    [shadow setShadowColor:skia::SkColorToSRGBNSColor(kShadowColor)];
    [shadow set];
    [outerColor set];

    [[NSBezierPath bezierPathWithRoundedRect:frame
                                     xRadius:kCornerRadius
                                     yRadius:kCornerRadius] fill];
  }

  [innerColor set];
  [[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(frame, 1, 1)
                                   xRadius:kCornerRadius
                                   yRadius:kCornerRadius] fill];
  [centerColor set];
  [[NSBezierPath bezierPathWithRoundedRect:NSInsetRect(frame, 2, 2)
                                   xRadius:kCornerRadius
                                   yRadius:kCornerRadius] fill];
}

@end
