// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/new_tab_button.h"

#include "base/mac/foundation_util.h"
#include "base/mac/sdk_forward_declarations.h"
#import "chrome/browser/ui/cocoa/image_button_cell.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#include "chrome/browser/ui/cocoa/tabs/tab_view.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/cocoa/nsgraphics_context_additions.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"

@class NewTabButtonCell;

namespace {

enum class OverlayOption {
  NONE,
  LIGHTEN,
  DARKEN,
};

const NSSize newTabButtonImageSize = {34, 18};

NSImage* GetMaskImageFromCell(NewTabButtonCell* aCell) {
  return [aCell imageForState:image_button_cell::kDefaultState view:nil];
}

// Creates an NSImage with size |size| and bitmap image representations for both
// 1x and 2x scale factors. |drawingHandler| is called once for every scale
// factor.  This is similar to -[NSImage imageWithSize:flipped:drawingHandler:],
// but this function always evaluates drawingHandler eagerly, and it works on
// 10.6 and 10.7.
NSImage* CreateImageWithSize(NSSize size,
                             void (^drawingHandler)(NSSize)) {
  base::scoped_nsobject<NSImage> result([[NSImage alloc] initWithSize:size]);
  [NSGraphicsContext saveGraphicsState];
  for (ui::ScaleFactor scale_factor : ui::GetSupportedScaleFactors()) {
    float scale = GetScaleForScaleFactor(scale_factor);
    NSBitmapImageRep *bmpImageRep = [[[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL
                      pixelsWide:size.width * scale
                      pixelsHigh:size.height * scale
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSDeviceRGBColorSpace
                     bytesPerRow:0
                    bitsPerPixel:0] autorelease];
    [bmpImageRep setSize:size];
    [NSGraphicsContext setCurrentContext:
        [NSGraphicsContext graphicsContextWithBitmapImageRep:bmpImageRep]];
    drawingHandler(size);
    [result addRepresentation:bmpImageRep];
  }
  [NSGraphicsContext restoreGraphicsState];

  return result.release();
}

// Takes a normal bitmap and a mask image and returns an image the size of the
// mask that has pixels from |image| but alpha information from |mask|.
NSImage* ApplyMask(NSImage* image, NSImage* mask) {
  return [CreateImageWithSize([mask size], ^(NSSize size) {
      // Skip a few pixels from the top of the tab background gradient, because
      // the new tab button is not drawn at the very top of the browser window.
      const int kYOffset = 10;
      CGFloat width = size.width;
      CGFloat height = size.height;

      // In some themes, the tab background image is narrower than the
      // new tab button, so tile the background image.
      CGFloat x = 0;
      // The floor() is to make sure images with odd widths don't draw to the
      // same pixel twice on retina displays. (Using NSDrawThreePartImage()
      // caused a startup perf regression, so that cannot be used.)
      CGFloat tileWidth = floor(std::min(width, [image size].width));
      while (x < width) {
        [image drawAtPoint:NSMakePoint(x, 0)
                  fromRect:NSMakeRect(0,
                                      [image size].height - height - kYOffset,
                                      tileWidth,
                                      height)
                 operation:NSCompositeCopy
                  fraction:1.0];
        x += tileWidth;
      }

      [mask drawAtPoint:NSZeroPoint
               fromRect:NSMakeRect(0, 0, width, height)
              operation:NSCompositeDestinationIn
               fraction:1.0];
  }) autorelease];
}

// Paints |overlay| on top of |ground|.
NSImage* Overlay(NSImage* ground, NSImage* overlay, CGFloat alpha) {
  DCHECK_EQ([ground size].width, [overlay size].width);
  DCHECK_EQ([ground size].height, [overlay size].height);

  return [CreateImageWithSize([ground size], ^(NSSize size) {
      CGFloat width = size.width;
      CGFloat height = size.height;
      [ground drawAtPoint:NSZeroPoint
                 fromRect:NSMakeRect(0, 0, width, height)
                operation:NSCompositeCopy
                 fraction:1.0];
      [overlay drawAtPoint:NSZeroPoint
                  fromRect:NSMakeRect(0, 0, width, height)
                 operation:NSCompositeSourceOver
                  fraction:alpha];
  }) autorelease];
}

CGFloat LineWidthFromContext(CGContextRef context) {
  CGRect unitRect = CGRectMake(0.0, 0.0, 1.0, 1.0);
  CGRect deviceRect = CGContextConvertRectToDeviceSpace(context, unitRect);
  return 1.0 / deviceRect.size.height;
}

}  // namespace

@interface NewTabButtonCustomImageRep : NSCustomImageRep
@property (assign, nonatomic) NSView* destView;
@property (copy, nonatomic) NSColor* fillColor;
@property (assign, nonatomic) NSPoint patternPhasePosition;
@property (assign, nonatomic) OverlayOption overlayOption;
@end

@implementation NewTabButtonCustomImageRep

@synthesize destView = destView_;
@synthesize fillColor = fillColor_;
@synthesize patternPhasePosition = patternPhasePosition_;
@synthesize overlayOption = overlayOption_;

- (void)dealloc {
  [fillColor_ release];
  [super dealloc];
}

@end

// A simple override of the ImageButtonCell to disable handling of
// -mouseEntered.
@interface NewTabButtonCell : ImageButtonCell

- (void)mouseEntered:(NSEvent*)theEvent;

@end

@implementation NewTabButtonCell

- (void)mouseEntered:(NSEvent*)theEvent {
  // Ignore this since the NTB enter is handled by the TabStripController.
}

- (void)drawFocusRingMaskWithFrame:(NSRect)cellFrame inView:(NSView*)view {
  // Match the button's shape.
  [self drawImage:GetMaskImageFromCell(self) withFrame:cellFrame inView:view];
}

@end

@interface NewTabButton()

// Returns a new tab button image appropriate for the specified button state
// (e.g. hover) and theme. In Material Design, the theme color affects the
// button color.
- (NSImage*)imageForState:(image_button_cell::ButtonState)state
                    theme:(const ui::ThemeProvider*)theme;

// Returns a new tab button image bezier path with the specified line width.
+ (NSBezierPath*)newTabButtonBezierPathWithLineWidth:(CGFloat)lineWidth;

// Draws the new tab button image to |imageRep|, with either a normal stroke or
// a heavy stroke for increased visibility.
+ (void)drawNewTabButtonImage:(NewTabButtonCustomImageRep*)imageRep
              withHeavyStroke:(BOOL)heavyStroke;

// NSCustomImageRep custom drawing method shims for normal and heavy strokes
// respectively.
+ (void)drawNewTabButtonImageWithNormalStroke:
    (NewTabButtonCustomImageRep*)imageRep;
+ (void)drawNewTabButtonImageWithHeavyStroke:
    (NewTabButtonCustomImageRep*)imageRep;

// Returns a new tab button image filled with |fillColor|.
- (NSImage*)imageWithFillColor:(NSColor*)fillColor;

@end

@implementation NewTabButton

+ (Class)cellClass {
  return [NewTabButtonCell class];
}

- (BOOL)pointIsOverButton:(NSPoint)point {
  NSPoint localPoint = [self convertPoint:point fromView:[self superview]];
  NSRect pointRect = NSMakeRect(localPoint.x, localPoint.y, 1, 1);
  NSImage* buttonMask = GetMaskImageFromCell([self cell]);
  NSRect bounds = self.bounds;
  NSSize buttonMaskSize = [buttonMask size];
  NSRect destinationRect = NSMakeRect(
      (NSWidth(bounds) - buttonMaskSize.width) / 2,
      (NSHeight(bounds) - buttonMaskSize.height) / 2,
      buttonMaskSize.width, buttonMaskSize.height);
  return [buttonMask hitTestRect:pointRect
        withImageDestinationRect:destinationRect
                         context:nil
                           hints:nil
                         flipped:YES];
}

// Override to only accept clicks within the bounds of the defined path, not
// the entire bounding box. |aPoint| is in the superview's coordinate system.
- (NSView*)hitTest:(NSPoint)aPoint {
  if ([self pointIsOverButton:aPoint])
    return [super hitTest:aPoint];
  return nil;
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  [self setNeedsDisplay:YES];
}

- (void)windowDidChangeActive {
  [self setNeedsDisplay:YES];
}

- (void)viewDidMoveToWindow {
  NewTabButtonCell* cell = base::mac::ObjCCast<NewTabButtonCell>([self cell]);

  if ([self window] &&
      ![cell imageForState:image_button_cell::kDefaultState view:self]) {
    [self setImages];
  }
}

- (void)setImages {
  const ui::ThemeProvider* theme = [[self window] themeProvider];
  if (!theme) {
    return;
  }

  NSImage* mask = [self imageWithFillColor:[NSColor whiteColor]];
  NSImage* normal =
      [self imageForState:image_button_cell::kDefaultState theme:theme];
  NSImage* hover =
      [self imageForState:image_button_cell::kHoverState theme:theme];
  NSImage* pressed =
      [self imageForState:image_button_cell::kPressedState theme:theme];
  NSImage* normalBackground = nil;
  NSImage* hoverBackground = nil;

  // If using a custom theme, overlay the default image with the theme's custom
  // tab background image.
  if (!theme->UsingSystemTheme()) {
    NSImage* foreground =
        ApplyMask(theme->GetNSImageNamed(IDR_THEME_TAB_BACKGROUND), mask);
    normal = Overlay(foreground, normal, 1.0);
    hover = Overlay(foreground, hover, 1.0);
    pressed = Overlay(foreground, pressed, 1.0);

    NSImage* background = ApplyMask(
        theme->GetNSImageNamed(IDR_THEME_TAB_BACKGROUND_INACTIVE), mask);
    normalBackground = Overlay(background, normal, tabs::kImageNoFocusAlpha);
    hoverBackground = Overlay(background, hover, tabs::kImageNoFocusAlpha);
  }

  NewTabButtonCell* cell = base::mac::ObjCCast<NewTabButtonCell>([self cell]);
  [cell setImage:normal forButtonState:image_button_cell::kDefaultState];
  [cell setImage:hover forButtonState:image_button_cell::kHoverState];
  [cell setImage:pressed forButtonState:image_button_cell::kPressedState];
  [cell setImage:normalBackground
      forButtonState:image_button_cell::kDefaultStateBackground];
  [cell setImage:hoverBackground
      forButtonState:image_button_cell::kHoverStateBackground];
}

- (NSImage*)imageForState:(image_button_cell::ButtonState)state
                    theme:(const ui::ThemeProvider*)theme {
  NSColor* fillColor = nil;
  OverlayOption overlayOption = OverlayOption::NONE;

  switch (state) {
    case image_button_cell::kDefaultState:
      // In the normal state, the NTP button looks like a background tab.
      fillColor = theme->GetNSImageColorNamed(IDR_THEME_TAB_BACKGROUND);
      break;

    case image_button_cell::kHoverState:
      fillColor = theme->GetNSImageColorNamed(IDR_THEME_TAB_BACKGROUND);
      overlayOption = OverlayOption::LIGHTEN;

      break;

    case image_button_cell::kPressedState:
      fillColor = theme->GetNSImageColorNamed(IDR_THEME_TAB_BACKGROUND);
      overlayOption = OverlayOption::DARKEN;
      break;

    case image_button_cell::kDefaultStateBackground:
    case image_button_cell::kHoverStateBackground:
      fillColor =
          theme->GetNSImageColorNamed(IDR_THEME_TAB_BACKGROUND_INACTIVE);
      break;

    default:
      fillColor = [NSColor redColor];
      // All states should be accounted for above.
      NOTREACHED();
  }

  SEL drawSelector = @selector(drawNewTabButtonImageWithNormalStroke:);
  if (theme && theme->ShouldIncreaseContrast())
    drawSelector = @selector(drawNewTabButtonImageWithHeavyStroke:);

  base::scoped_nsobject<NewTabButtonCustomImageRep> imageRep(
      [[NewTabButtonCustomImageRep alloc]
          initWithDrawSelector:drawSelector
                      delegate:[NewTabButton class]]);
  [imageRep setDestView:self];
  [imageRep setFillColor:fillColor];
  [imageRep setPatternPhasePosition:
      [[self window]
          themeImagePositionForAlignment:THEME_IMAGE_ALIGN_WITH_TAB_STRIP]];
  [imageRep setOverlayOption:overlayOption];

  NSImage* newTabButtonImage =
      [[[NSImage alloc] initWithSize:newTabButtonImageSize] autorelease];
  [newTabButtonImage addRepresentation:imageRep];

  return newTabButtonImage;
}

+ (NSBezierPath*)newTabButtonBezierPathWithLineWidth:(CGFloat)lineWidth {
  NSBezierPath* bezierPath = [NSBezierPath bezierPath];

  // This data comes straight from the SVG.
  [bezierPath moveToPoint:NSMakePoint(15.2762236,30)];

  [bezierPath curveToPoint:NSMakePoint(11.0354216,27.1770115)
             controlPoint1:NSMakePoint(13.3667706,30)
             controlPoint2:NSMakePoint(11.7297681,28.8344828)];

  [bezierPath curveToPoint:NSMakePoint(7.28528951e-08,2.01431416)
             controlPoint1:NSMakePoint(11.0354216,27.1770115)
             controlPoint2:NSMakePoint(0.000412425082,3.87955717)];

  [bezierPath curveToPoint:NSMakePoint(1.70510791,0)
             controlPoint1:NSMakePoint(-0.000270516213,0.790325707)
             controlPoint2:NSMakePoint(0.753255356,0)];

  [bezierPath lineToPoint:NSMakePoint(48.7033642,0)];

  [bezierPath curveToPoint:NSMakePoint(52.9464653,2.82643678)
             controlPoint1:NSMakePoint(50.6151163,0)
             controlPoint2:NSMakePoint(52.2521188,1.16666667)];

  [bezierPath curveToPoint:NSMakePoint(64.0268555,27.5961914)
             controlPoint1:NSMakePoint(52.9464653,2.82643678)
             controlPoint2:NSMakePoint(64.0268555,27.4111339)];

  [bezierPath curveToPoint:NSMakePoint(62.2756294,30)
             controlPoint1:NSMakePoint(64.0268555,28.5502144)
             controlPoint2:NSMakePoint(63.227482,29.9977011)];

  [bezierPath closePath];

  // The SVG path is flipped for some reason, so flip it back. However, in RTL,
  // we'd need to flip it again below, so when in RTL mode just leave the flip
  // out altogether.
  if (!cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    const CGFloat kSVGHeight = 32;
    NSAffineTransformStruct flipStruct = {1, 0, 0, -1, 0, kSVGHeight};
    NSAffineTransform* flipTransform = [NSAffineTransform transform];
    [flipTransform setTransformStruct:flipStruct];
    [bezierPath transformUsingAffineTransform:flipTransform];
  }

  // The SVG data is for the 2x version so scale it down.
  NSAffineTransform* scaleTransform = [NSAffineTransform transform];
  const CGFloat k50PercentScale = 0.5;
  [scaleTransform scaleBy:k50PercentScale];
  [bezierPath transformUsingAffineTransform:scaleTransform];

  // Adjust by half the line width to get crisp lines.
  NSAffineTransform* transform = [NSAffineTransform transform];
  [transform translateXBy:lineWidth / 2 yBy:lineWidth / 2];
  [bezierPath transformUsingAffineTransform:transform];

  [bezierPath setLineWidth:lineWidth];

  return bezierPath;
}

+ (void)drawNewTabButtonImage:(NewTabButtonCustomImageRep*)imageRep
              withHeavyStroke:(BOOL)heavyStroke {
  [[NSGraphicsContext currentContext]
      cr_setPatternPhase:[imageRep patternPhasePosition]
      forView:[imageRep destView]];

  CGContextRef context = static_cast<CGContextRef>(
      [[NSGraphicsContext currentContext] graphicsPort]);
  CGFloat lineWidth = LineWidthFromContext(context);
  NSBezierPath* bezierPath =
      [self newTabButtonBezierPathWithLineWidth:lineWidth];

  if ([imageRep fillColor]) {
    [[imageRep fillColor] set];
    [bezierPath fill];
  }

  CGFloat alpha = heavyStroke ? 1.0 : 0.25;
  static NSColor* strokeColor =
      [[NSColor colorWithCalibratedWhite:0 alpha:alpha] retain];
  [strokeColor set];
  [bezierPath stroke];

  BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
  CGFloat buttonWidth = newTabButtonImageSize.width;

  // Bottom edge.
  const CGFloat kBottomEdgeX = 9;
  const CGFloat kBottomEdgeY = 1.2825;
  const CGFloat kBottomEdgeWidth = 22;
  NSPoint bottomEdgeStart = NSMakePoint(kBottomEdgeX, kBottomEdgeY);
  NSPoint bottomEdgeEnd = NSMakePoint(kBottomEdgeX + kBottomEdgeWidth,
                                      kBottomEdgeY);
  if (isRTL) {
    bottomEdgeStart.x = buttonWidth - bottomEdgeStart.x;
    bottomEdgeEnd.x = buttonWidth - bottomEdgeEnd.x;
  }
  NSBezierPath* bottomEdgePath = [NSBezierPath bezierPath];
  [bottomEdgePath moveToPoint:bottomEdgeStart];
  [bottomEdgePath lineToPoint:bottomEdgeEnd];
  static NSColor* bottomEdgeColor =
      [[NSColor colorWithCalibratedWhite:0 alpha:0.07] retain];
  [bottomEdgeColor set];
  [bottomEdgePath setLineWidth:lineWidth];
  [bottomEdgePath setLineCapStyle:NSRoundLineCapStyle];
  [bottomEdgePath stroke];

  CGPoint shadowStart = NSZeroPoint;
  CGPoint shadowEnd = NSZeroPoint;
  NSColor* overlayColor = nil;
  const CGFloat kBottomShadowX = 8;
  const CGFloat kBottomShadowY = kBottomEdgeY - lineWidth;
  const CGFloat kTopShadowX = 1;
  const CGFloat kTopShadowY = kBottomShadowY + 15;
  const CGFloat kShadowWidth = 24;
  static NSColor* lightOverlayColor =
      [[NSColor colorWithCalibratedWhite:1 alpha:0.20] retain];
  static NSColor* darkOverlayColor =
      [[NSColor colorWithCalibratedWhite:0 alpha:0.08] retain];

  switch ([imageRep overlayOption]) {
    case OverlayOption::LIGHTEN:
      overlayColor = lightOverlayColor;
      break;

    case OverlayOption::DARKEN:
      overlayColor = darkOverlayColor;
      shadowStart = NSMakePoint(kTopShadowX, kTopShadowY);
      shadowEnd = NSMakePoint(kTopShadowX + kShadowWidth, kTopShadowY);
      break;

    case OverlayOption::NONE:
      shadowStart = NSMakePoint(kBottomShadowX, kBottomShadowY);
      shadowEnd = NSMakePoint(kBottomShadowX + kShadowWidth, kBottomShadowY);
      break;
  }

  // Shadow beneath the bottom or top edge.
  if (!NSEqualPoints(shadowStart, NSZeroPoint)) {
    if (isRTL) {
      shadowStart.x = buttonWidth - shadowStart.x;
      shadowEnd.x = buttonWidth - shadowEnd.x;
    }
    NSBezierPath* shadowPath = [NSBezierPath bezierPath];
    [shadowPath moveToPoint:shadowStart];
    [shadowPath lineToPoint:shadowEnd];
    [shadowPath setLineWidth:lineWidth];
    [shadowPath setLineCapStyle:NSRoundLineCapStyle];
    static NSColor* shadowColor =
        [[NSColor colorWithCalibratedWhite:0 alpha:0.10] retain];
    [shadowColor set];
    [shadowPath stroke];
  }

  if (overlayColor) {
    [overlayColor set];
    [[self newTabButtonBezierPathWithLineWidth:lineWidth] fill];
  }
}

+ (void)drawNewTabButtonImageWithNormalStroke:
    (NewTabButtonCustomImageRep*)image {
  [self drawNewTabButtonImage:image withHeavyStroke:NO];
}

+ (void)drawNewTabButtonImageWithHeavyStroke:
    (NewTabButtonCustomImageRep*)image {
  [self drawNewTabButtonImage:image withHeavyStroke:YES];
}

- (NSImage*)imageWithFillColor:(NSColor*)fillColor {
  NSImage* image =
      [[[NSImage alloc] initWithSize:newTabButtonImageSize] autorelease];

  [image lockFocus];
  [fillColor set];
  CGContextRef context = static_cast<CGContextRef>(
      [[NSGraphicsContext currentContext] graphicsPort]);
  CGFloat lineWidth = LineWidthFromContext(context);
  [[NewTabButton newTabButtonBezierPathWithLineWidth:lineWidth] fill];
  [image unlockFocus];
  return image;
}

@end
