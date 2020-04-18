// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/download_item_cell.h"

#include "base/bind.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/download/download_shelf.h"
#import "chrome/browser/themes/theme_properties.h"
#import "chrome/browser/ui/cocoa/download/background_theme.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/grit/theme_resources.h"
#include "components/download/public/common/download_item.h"
#include "content/public/browser/download_manager.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSColor+Luminance.h"
#include "ui/base/default_theme_provider.h"
#include "ui/gfx/canvas_skia_paint.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/text_elider.h"
#include "ui/native_theme/native_theme.h"

// Distance from top border to icon.
const CGFloat kImagePaddingTop = 7;

// Distance from left border to icon.
const CGFloat kImagePaddingLeft = 9;

// Horizontal distance from the icon to the text.
const CGFloat kImagePaddingRight = 7;

// Width of icon.
const CGFloat kImageWidth = 16;

// Height of icon.
const CGFloat kImageHeight = 16;

// x coordinate of download name string, in view coords.
const CGFloat kTextPosLeft = kImagePaddingLeft +
    kImageWidth + DownloadShelf::kFiletypeIconOffset + kImagePaddingRight;

// Distance from end of download name string to dropdown area.
const CGFloat kTextPaddingRight = 3;

// y coordinate of download name string, in view coords, when status message
// is visible.
const CGFloat kPrimaryTextPosTop = 3;

// y coordinate of download name string, in view coords, when status message
// is not visible.
const CGFloat kPrimaryTextOnlyPosTop = 10;

// y coordinate of status message, in view coords.
const CGFloat kSecondaryTextPosTop = 18;

// Width of dropdown area on the right (includes 1px for the border on each
// side).
const CGFloat kDropdownAreaWidth = 14;

// Width of dropdown arrow.
const CGFloat kDropdownArrowWidth = 5;

// Height of dropdown arrow.
const CGFloat kDropdownArrowHeight = 3;

// Vertical displacement of dropdown area, relative to the "centered" position.
const CGFloat kDropdownAreaY = -2;

// Duration of the two-lines-to-one-line animation, in seconds.
NSTimeInterval kShowStatusDuration = 0.3;
NSTimeInterval kHideStatusDuration = 0.3;

// Duration of the 'download complete' animation, in seconds.
const CGFloat kCompleteAnimationDuration = 2.5;

// Duration of the 'download interrupted' animation, in seconds.
const CGFloat kInterruptedAnimationDuration = 2.5;

using download::DownloadItem;

// This is a helper class to animate the fading out of the status text.
@interface DownloadItemCellAnimation : NSAnimation {
 @private
  DownloadItemCell* cell_;
}
- (id)initWithDownloadItemCell:(DownloadItemCell*)cell
                      duration:(NSTimeInterval)duration
                animationCurve:(NSAnimationCurve)animationCurve;

@end

// Timer used to animate indeterminate progress. An NSTimer retains its target.
// This means that the target must explicitly invalidate the timer before it
// can be deleted. This class keeps a weak reference to the target so the
// timer can be invalidated from the destructor.
@interface IndeterminateProgressTimer : NSObject {
 @private
  DownloadItemCell* cell_;
  base::scoped_nsobject<NSTimer> timer_;
}

- (id)initWithDownloadItemCell:(DownloadItemCell*)cell;
- (void)invalidate;

@end

@interface DownloadItemCell(Private)
- (void)updateTrackingAreas:(id)sender;
- (void)setupToggleStatusVisibilityAnimation;
- (void)showSecondaryTitle;
- (void)hideSecondaryTitle;
- (void)animation:(NSAnimation*)animation
       progressed:(NSAnimationProgress)progress;
- (void)updateIndeterminateDownload;
- (void)stopIndeterminateAnimation;
- (NSString*)elideTitle:(int)availableWidth;
- (NSString*)elideStatus:(int)availableWidth;
- (const ui::ThemeProvider*)backgroundThemeWrappingProvider:
    (const ui::ThemeProvider*)provider;
- (BOOL)pressedWithDefaultThemeOnPart:(DownloadItemMousePosition)part;
- (NSColor*)titleColorForPart:(DownloadItemMousePosition)part;
- (void)drawSecondaryTitleInRect:(NSRect)innerFrame;
- (BOOL)isDefaultTheme;
@end

@implementation DownloadItemCell

@synthesize secondaryTitle = secondaryTitle_;
@synthesize secondaryFont = secondaryFont_;

- (void)setInitialState {
  isStatusTextVisible_ = NO;
  titleY_ = kPrimaryTextOnlyPosTop;
  statusAlpha_ = 0.0;

  [self setFont:[NSFont systemFontOfSize:
      [NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
  [self setSecondaryFont:[NSFont systemFontOfSize:
      [NSFont systemFontSizeForControlSize:NSSmallControlSize]]];

  [self updateTrackingAreas:self];
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(updateTrackingAreas:)
             name:NSViewFrameDidChangeNotification
           object:[self controlView]];
}

// For nib instantiations
- (id)initWithCoder:(NSCoder*)decoder {
  if ((self = [super initWithCoder:decoder])) {
    [self setInitialState];
  }
  return self;
}

// For programmatic instantiations.
- (id)initTextCell:(NSString *)string {
  if ((self = [super initTextCell:string])) {
    [self setInitialState];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  if ([completionAnimation_ isAnimating])
    [completionAnimation_ stopAnimation];
  if ([toggleStatusVisibilityAnimation_ isAnimating])
    [toggleStatusVisibilityAnimation_ stopAnimation];
  if (trackingAreaButton_) {
    [[self controlView] removeTrackingArea:trackingAreaButton_];
    trackingAreaButton_.reset();
  }
  if (trackingAreaDropdown_) {
    [[self controlView] removeTrackingArea:trackingAreaDropdown_];
    trackingAreaDropdown_.reset();
  }
  [self stopIndeterminateAnimation];
  [secondaryTitle_ release];
  [secondaryFont_ release];
  [super dealloc];
}

- (void)setStateFromDownload:(DownloadItemModel*)downloadModel {
  // Set the name of the download.
  downloadPath_ = downloadModel->download()->GetFileNameToReportUser();

  base::string16 statusText = downloadModel->GetStatusText();
  if (statusText.empty()) {
    // Remove the status text label.
    [self hideSecondaryTitle];
  } else {
    // Set status text.
    NSString* statusString = base::SysUTF16ToNSString(statusText);
    [self setSecondaryTitle:statusString];
    [self showSecondaryTitle];
  }

  switch (downloadModel->download()->GetState()) {
    case DownloadItem::COMPLETE:
      // Small downloads may start in a complete state due to asynchronous
      // notifications. In this case, we'll get a second complete notification
      // via the observers, so we ignore it and avoid creating a second complete
      // animation.
      if (completionAnimation_.get())
        break;
      completionAnimation_.reset([[DownloadItemCellAnimation alloc]
          initWithDownloadItemCell:self
                          duration:kCompleteAnimationDuration
                    animationCurve:NSAnimationLinear]);
      [completionAnimation_.get() setDelegate:self];
      [completionAnimation_.get() startAnimation];
      percentDone_ = -1;
      [self stopIndeterminateAnimation];
      break;
    case DownloadItem::CANCELLED:
      percentDone_ = -1;
      [self stopIndeterminateAnimation];
      break;
    case DownloadItem::INTERRUPTED:
      // Small downloads may start in an interrupted state due to asynchronous
      // notifications. In this case, we'll get a second complete notification
      // via the observers, so we ignore it and avoid creating a second complete
      // animation.
      if (completionAnimation_.get())
        break;
      completionAnimation_.reset([[DownloadItemCellAnimation alloc]
          initWithDownloadItemCell:self
                          duration:kInterruptedAnimationDuration
                    animationCurve:NSAnimationLinear]);
      [completionAnimation_.get() setDelegate:self];
      [completionAnimation_.get() startAnimation];
      percentDone_ = -2;
      [self stopIndeterminateAnimation];
      break;
    case DownloadItem::IN_PROGRESS:
      if (downloadModel->download()->IsPaused()) {
        percentDone_ = -1;
        [self stopIndeterminateAnimation];
      } else if (downloadModel->PercentComplete() == -1) {
        percentDone_ = -1;
        if (!indeterminateProgressTimer_) {
          indeterminateProgressTimer_.reset([[IndeterminateProgressTimer alloc]
              initWithDownloadItemCell:self]);
          progressStartTime_ = base::TimeTicks::Now();
        }
      } else {
        percentDone_ = downloadModel->PercentComplete();
        [self stopIndeterminateAnimation];
      }
      break;
    default:
      NOTREACHED();
  }

  [[self controlView] setNeedsDisplay:YES];
}

- (void)updateTrackingAreas:(id)sender {
  if (trackingAreaButton_) {
    [[self controlView] removeTrackingArea:trackingAreaButton_.get()];
      trackingAreaButton_.reset(nil);
  }
  if (trackingAreaDropdown_) {
    [[self controlView] removeTrackingArea:trackingAreaDropdown_.get()];
      trackingAreaDropdown_.reset(nil);
  }

  // Use two distinct tracking rects for left and right parts.
  // The tracking areas are also used to decide how to handle clicks. They must
  // always be active, so the click is handled correctly when a download item
  // is clicked while chrome is not the active app ( http://crbug.com/21916 ).
  NSRect bounds = [[self controlView] bounds];
  NSRect buttonRect, dropdownRect;
  NSDivideRect(bounds, &dropdownRect, &buttonRect,
      kDropdownAreaWidth, NSMaxXEdge);

  trackingAreaButton_.reset([[NSTrackingArea alloc]
                  initWithRect:buttonRect
                       options:(NSTrackingMouseEnteredAndExited |
                                NSTrackingActiveAlways)
                         owner:self
                    userInfo:nil]);
  [[self controlView] addTrackingArea:trackingAreaButton_.get()];

  trackingAreaDropdown_.reset([[NSTrackingArea alloc]
                  initWithRect:dropdownRect
                       options:(NSTrackingMouseEnteredAndExited |
                                NSTrackingActiveAlways)
                         owner:self
                    userInfo:nil]);
  [[self controlView] addTrackingArea:trackingAreaDropdown_.get()];
}

- (void)setShowsBorderOnlyWhileMouseInside:(BOOL)showOnly {
  // Override to make sure it doesn't do anything if it's called accidentally.
}

- (void)mouseEntered:(NSEvent*)theEvent {
  mouseInsideCount_++;
  if ([theEvent trackingArea] == trackingAreaButton_.get())
    mousePosition_ = kDownloadItemMouseOverButtonPart;
  else if ([theEvent trackingArea] == trackingAreaDropdown_.get())
    mousePosition_ = kDownloadItemMouseOverDropdownPart;
  [[self controlView] setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent *)theEvent {
  mouseInsideCount_--;
  if (mouseInsideCount_ == 0)
    mousePosition_ = kDownloadItemMouseOutside;
  [[self controlView] setNeedsDisplay:YES];
}

- (BOOL)isMouseInside {
  return mousePosition_ != kDownloadItemMouseOutside;
}

- (BOOL)isMouseOverButtonPart {
  return mousePosition_ == kDownloadItemMouseOverButtonPart;
}

- (BOOL)isButtonPartPressed {
  return [self isHighlighted]
      && mousePosition_ == kDownloadItemMouseOverButtonPart;
}

- (BOOL)isMouseOverDropdownPart {
  return mousePosition_ == kDownloadItemMouseOverDropdownPart;
}

- (BOOL)isDropdownPartPressed {
  return [self isHighlighted]
      && mousePosition_ == kDownloadItemMouseOverDropdownPart;
}

- (NSBezierPath*)leftRoundedPath:(CGFloat)radius inRect:(NSRect)rect {

  NSPoint topLeft = NSMakePoint(NSMinX(rect), NSMaxY(rect));
  NSPoint topRight = NSMakePoint(NSMaxX(rect), NSMaxY(rect));
  NSPoint bottomRight = NSMakePoint(NSMaxX(rect) , NSMinY(rect));

  NSBezierPath* path = [NSBezierPath bezierPath];
  [path moveToPoint:topRight];
  [path appendBezierPathWithArcFromPoint:topLeft
                                 toPoint:rect.origin
                                  radius:radius];
  [path appendBezierPathWithArcFromPoint:rect.origin
                                 toPoint:bottomRight
                                 radius:radius];
  [path lineToPoint:bottomRight];
  return path;
}

- (NSBezierPath*)rightRoundedPath:(CGFloat)radius inRect:(NSRect)rect {

  NSPoint topLeft = NSMakePoint(NSMinX(rect), NSMaxY(rect));
  NSPoint topRight = NSMakePoint(NSMaxX(rect), NSMaxY(rect));
  NSPoint bottomRight = NSMakePoint(NSMaxX(rect), NSMinY(rect));

  NSBezierPath* path = [NSBezierPath bezierPath];
  [path moveToPoint:rect.origin];
  [path appendBezierPathWithArcFromPoint:bottomRight
                                toPoint:topRight
                                  radius:radius];
  [path appendBezierPathWithArcFromPoint:topRight
                                toPoint:topLeft
                                 radius:radius];
  [path lineToPoint:topLeft];
  return path;
}

- (NSString*)elideTitle:(int)availableWidth {
  return base::SysUTF16ToNSString(
      gfx::ElideFilename(downloadPath_, gfx::FontList(gfx::Font([self font])),
                         availableWidth, gfx::Typesetter::BROWSER));
}

- (NSString*)elideStatus:(int)availableWidth {
  return base::SysUTF16ToNSString(gfx::ElideText(
      base::SysNSStringToUTF16([self secondaryTitle]),
      gfx::FontList(gfx::Font([self secondaryFont])), availableWidth,
      gfx::ELIDE_TAIL, gfx::Typesetter::BROWSER));
}

- (const ui::ThemeProvider*)backgroundThemeWrappingProvider:
    (const ui::ThemeProvider*)provider {
  if (!themeProvider_.get()) {
    themeProvider_.reset(new BackgroundTheme(provider));
  }

  return themeProvider_.get();
}

// Returns if |part| was pressed while the default theme was active.
- (BOOL)pressedWithDefaultThemeOnPart:(DownloadItemMousePosition)part {
  return [self isDefaultTheme] && [self isHighlighted] &&
          mousePosition_ == part;
}

// Returns the text color that should be used to draw text on |part|.
- (NSColor*)titleColorForPart:(DownloadItemMousePosition)part {
  const ui::ThemeProvider* themeProvider =
      [[[self controlView] window] themeProvider];
  if ([self pressedWithDefaultThemeOnPart:part] || !themeProvider)
    return [NSColor alternateSelectedControlTextColor];
  return themeProvider->GetNSColor(ThemeProperties::COLOR_BOOKMARK_TEXT);
}

- (void)drawSecondaryTitleInRect:(NSRect)innerFrame {
  if (![self secondaryTitle] || statusAlpha_ <= 0)
    return;

  CGFloat textWidth = NSWidth(innerFrame) -
      (kTextPosLeft + kTextPaddingRight + kDropdownAreaWidth);
  NSString* secondaryText = [self elideStatus:textWidth];
  NSColor* secondaryColor =
      [self titleColorForPart:kDownloadItemMouseOverButtonPart];

  // If text is light-on-dark, lightening it alone will do nothing.
  // Therefore we mute luminance a wee bit before drawing in this case.
  if (![secondaryColor gtm_isDarkColor])
    secondaryColor = [secondaryColor gtm_colorByAdjustingLuminance:-0.2];

  NSDictionary* secondaryTextAttributes =
      [NSDictionary dictionaryWithObjectsAndKeys:
          secondaryColor, NSForegroundColorAttributeName,
          [self secondaryFont], NSFontAttributeName,
          nil];
  NSPoint secondaryPos =
      NSMakePoint(innerFrame.origin.x + kTextPosLeft, kSecondaryTextPosTop);

  gfx::ScopedNSGraphicsContextSaveGState contextSave;
  NSGraphicsContext* nsContext = [NSGraphicsContext currentContext];
  CGContextRef cgContext = (CGContextRef)[nsContext graphicsPort];
  [nsContext setCompositingOperation:NSCompositeSourceOver];
  CGContextSetAlpha(cgContext, statusAlpha_);
  [secondaryText drawAtPoint:secondaryPos
              withAttributes:secondaryTextAttributes];
}

- (BOOL)isDefaultTheme {
  const ui::ThemeProvider* themeProvider =
      [[[self controlView] window] themeProvider];
  if (!themeProvider)
    return YES;
  return !themeProvider->HasCustomImage(IDR_THEME_BUTTON_BACKGROUND);
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  NSRect drawFrame = NSInsetRect(cellFrame, 1.5, 1.5);
  NSRect innerFrame = NSInsetRect(cellFrame, 2, 2);

  const float radius = 3;
  NSWindow* window = [controlView window];
  BOOL active = [window isKeyWindow] || [window isMainWindow];

  // In the default theme, draw download items with the bookmark button
  // gradient. For some themes, this leads to unreadable text, so draw the item
  // with a background that looks like windows (some transparent white) if a
  // theme is used. Use custom theme object with a white color gradient to trick
  // the superclass into drawing what we want.
  const ui::ThemeProvider* themeProvider =
      [[[self controlView] window] themeProvider];

  NSGradient* bgGradient = nil;
  if (![self isDefaultTheme]) {
    themeProvider = [self backgroundThemeWrappingProvider:themeProvider];
    bgGradient = themeProvider->GetNSGradient(
        active ? ThemeProperties::GRADIENT_TOOLBAR_BUTTON :
                 ThemeProperties::GRADIENT_TOOLBAR_BUTTON_INACTIVE);
  }

  NSRect buttonDrawRect, dropdownDrawRect;
  NSDivideRect(drawFrame, &dropdownDrawRect, &buttonDrawRect,
      kDropdownAreaWidth, NSMaxXEdge);

  NSBezierPath* buttonInnerPath = [self
      leftRoundedPath:radius inRect:buttonDrawRect];
  NSBezierPath* dropdownInnerPath = [self
      rightRoundedPath:radius inRect:dropdownDrawRect];

  // Draw secondary title, if any. Do this before drawing the (transparent)
  // fill so that the text becomes a bit lighter. The default theme's "pressed"
  // gradient is not transparent, so only do this if a theme is active.
  bool drawStatusOnTop =
      [self pressedWithDefaultThemeOnPart:kDownloadItemMouseOverButtonPart];
  if (!drawStatusOnTop)
    [self drawSecondaryTitleInRect:innerFrame];

  // Stroke the borders and appropriate fill gradient.
  [self drawBorderAndFillForTheme:themeProvider
                      controlView:controlView
                        innerPath:buttonInnerPath
              showClickedGradient:[self isButtonPartPressed]
            showHighlightGradient:[self isMouseOverButtonPart]
                       hoverAlpha:0.0
                           active:active
                        cellFrame:cellFrame
                  defaultGradient:bgGradient];

  [self drawBorderAndFillForTheme:themeProvider
                      controlView:controlView
                        innerPath:dropdownInnerPath
              showClickedGradient:[self isDropdownPartPressed]
            showHighlightGradient:[self isMouseOverDropdownPart]
                       hoverAlpha:0.0
                           active:active
                        cellFrame:cellFrame
                  defaultGradient:bgGradient];

  [self drawInteriorWithFrame:innerFrame inView:controlView];

  // For the default theme, draw the status text on top of the (opaque) button
  // gradient.
  if (drawStatusOnTop)
    [self drawSecondaryTitleInRect:innerFrame];
}

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  // Draw title
  CGFloat textWidth = NSWidth(cellFrame) -
      (kTextPosLeft + kTextPaddingRight + kDropdownAreaWidth);
  [self setTitle:[self elideTitle:textWidth]];

  NSColor* color = [self titleColorForPart:kDownloadItemMouseOverButtonPart];
  NSString* primaryText = [self title];

  NSDictionary* primaryTextAttributes =
      [NSDictionary dictionaryWithObjectsAndKeys:
          color, NSForegroundColorAttributeName,
          [self font], NSFontAttributeName,
          nil];
  NSPoint primaryPos = NSMakePoint(
      cellFrame.origin.x + kTextPosLeft,
      titleY_);

  [primaryText drawAtPoint:primaryPos withAttributes:primaryTextAttributes];

  // Draw progress disk
  {
    // CanvasSkiaPaint draws its content to the current NSGraphicsContext in its
    // destructor, which needs to be invoked before the icon is drawn below -
    // hence this nested block.

    // Always repaint the whole disk.
    NSPoint imagePosition = [self imageRectForBounds:cellFrame].origin;
    int x = imagePosition.x - DownloadShelf::kFiletypeIconOffset;
    int y = imagePosition.y - DownloadShelf::kFiletypeIconOffset;
    NSRect dirtyRect = NSMakeRect(
        x - 1, y - 1,
        DownloadShelf::kProgressIndicatorSize + 2,
        DownloadShelf::kProgressIndicatorSize + 2);

    gfx::CanvasSkiaPaint canvas(dirtyRect, false);
    canvas.set_composite_alpha(true);
    canvas.Translate(gfx::Vector2d(x, y));

    const ui::ThemeProvider* themeProvider =
        [[[self controlView] window] themeProvider];
    ui::DefaultThemeProvider defaultTheme;
    if (!themeProvider)
      themeProvider = &defaultTheme;

    if (completionAnimation_.get()) {
      if ([completionAnimation_ isAnimating]) {
        if (percentDone_ == -1) {
          DownloadShelf::PaintDownloadComplete(
              &canvas, *themeProvider,
              [completionAnimation_ currentValue]);
        } else {
          DownloadShelf::PaintDownloadInterrupted(
              &canvas, *themeProvider,
              [completionAnimation_ currentValue]);
        }
      }
    } else if (percentDone_ >= 0 || indeterminateProgressTimer_) {
      DownloadShelf::PaintDownloadProgress(
          &canvas, *themeProvider,
          base::TimeTicks::Now() - progressStartTime_, percentDone_);
    }
  }

  // Draw icon
  [[self image] drawInRect:[self imageRectForBounds:cellFrame]
                  fromRect:NSZeroRect
                 operation:NSCompositeSourceOver
                  fraction:[self isEnabled] ? 1.0 : 0.5
            respectFlipped:YES
                     hints:nil];

  // Separator between button and popup parts
  CGFloat lx = NSMaxX(cellFrame) - kDropdownAreaWidth + 0.5;
  [[NSColor colorWithDeviceWhite:0.0 alpha:0.1] set];
  [NSBezierPath strokeLineFromPoint:NSMakePoint(lx, NSMinY(cellFrame) + 1)
                            toPoint:NSMakePoint(lx, NSMaxY(cellFrame) - 1)];
  [[NSColor colorWithDeviceWhite:1.0 alpha:0.1] set];
  [NSBezierPath strokeLineFromPoint:NSMakePoint(lx + 1, NSMinY(cellFrame) + 1)
                            toPoint:NSMakePoint(lx + 1, NSMaxY(cellFrame) - 1)];

  // Popup arrow. Put center of mass of the arrow in the center of the
  // dropdown area.
  CGFloat cx = NSMaxX(cellFrame) - kDropdownAreaWidth/2 + 0.5;
  CGFloat cy = NSMidY(cellFrame);
  NSPoint p1 = NSMakePoint(cx - kDropdownArrowWidth/2,
                           cy - kDropdownArrowHeight/3 + kDropdownAreaY);
  NSPoint p2 = NSMakePoint(cx + kDropdownArrowWidth/2,
                           cy - kDropdownArrowHeight/3 + kDropdownAreaY);
  NSPoint p3 = NSMakePoint(cx, cy + kDropdownArrowHeight*2/3 + kDropdownAreaY);
  NSBezierPath *triangle = [NSBezierPath bezierPath];
  [triangle moveToPoint:p1];
  [triangle lineToPoint:p2];
  [triangle lineToPoint:p3];
  [triangle closePath];

  gfx::ScopedNSGraphicsContextSaveGState scopedGState;

  base::scoped_nsobject<NSShadow> shadow([[NSShadow alloc] init]);
  [shadow.get() setShadowColor:[NSColor whiteColor]];
  [shadow.get() setShadowOffset:NSMakeSize(0, -1)];
  [shadow setShadowBlurRadius:0.0];
  [shadow set];

  NSColor* fill = [self titleColorForPart:kDownloadItemMouseOverDropdownPart];
  [fill setFill];

  [triangle fill];
}

- (NSRect)imageRectForBounds:(NSRect)cellFrame {
  return NSMakeRect(cellFrame.origin.x + kImagePaddingLeft,
                    cellFrame.origin.y + kImagePaddingTop,
                    kImageWidth,
                    kImageHeight);
}

- (void)setupToggleStatusVisibilityAnimation {
  if (toggleStatusVisibilityAnimation_ &&
      [toggleStatusVisibilityAnimation_ isAnimating]) {
    // If the animation is running, cancel the animation and show/hide the
    // status text immediately.
    [toggleStatusVisibilityAnimation_ stopAnimation];
    [self animation:toggleStatusVisibilityAnimation_ progressed:1.0];
    toggleStatusVisibilityAnimation_.reset();
  } else {
    // Don't use core animation -- text in CA layers is not subpixel antialiased
    toggleStatusVisibilityAnimation_.reset([[DownloadItemCellAnimation alloc]
        initWithDownloadItemCell:self
                        duration:kShowStatusDuration
                  animationCurve:NSAnimationEaseIn]);
    [toggleStatusVisibilityAnimation_.get() setDelegate:self];
    [toggleStatusVisibilityAnimation_.get() startAnimation];
  }
}

- (void)showSecondaryTitle {
  if (isStatusTextVisible_)
    return;
  isStatusTextVisible_ = YES;
  [self setupToggleStatusVisibilityAnimation];
}

- (void)hideSecondaryTitle {
  if (!isStatusTextVisible_)
    return;
  isStatusTextVisible_ = NO;
  [self setupToggleStatusVisibilityAnimation];
}

- (IndeterminateProgressTimer*)indeterminateProgressTimer {
  return indeterminateProgressTimer_;
}

- (void)animation:(NSAnimation*)animation
   progressed:(NSAnimationProgress)progress {
  if (animation == toggleStatusVisibilityAnimation_) {
    if (isStatusTextVisible_) {
      titleY_ = (1 - progress)*kPrimaryTextOnlyPosTop + kPrimaryTextPosTop;
      statusAlpha_ = progress;
    } else {
      titleY_ = progress*kPrimaryTextOnlyPosTop +
          (1 - progress)*kPrimaryTextPosTop;
      statusAlpha_ = 1 - progress;
    }
    [[self controlView] setNeedsDisplay:YES];
  } else if (animation == completionAnimation_) {
    [[self controlView] setNeedsDisplay:YES];
  }
}

- (void)updateIndeterminateDownload {
  [[self controlView] setNeedsDisplay:YES];
}

- (void)stopIndeterminateAnimation {
  [indeterminateProgressTimer_ invalidate];
  indeterminateProgressTimer_.reset();
}

- (void)animationDidEnd:(NSAnimation *)animation {
  if (animation == toggleStatusVisibilityAnimation_)
    toggleStatusVisibilityAnimation_.reset();
  else if (animation == completionAnimation_)
    completionAnimation_.reset();
}

- (BOOL)isStatusTextVisible {
  return isStatusTextVisible_;
}

- (CGFloat)statusTextAlpha {
  return statusAlpha_;
}

- (CGFloat)titleY {
  return titleY_;
}

- (void)skipVisibilityAnimation {
  [toggleStatusVisibilityAnimation_ setCurrentProgress:1.0];
}

@end

@implementation DownloadItemCellAnimation

- (id)initWithDownloadItemCell:(DownloadItemCell*)cell
                      duration:(NSTimeInterval)duration
                animationCurve:(NSAnimationCurve)animationCurve {
  if ((self = [super gtm_initWithDuration:duration
                                eventMask:NSLeftMouseDownMask
                           animationCurve:animationCurve])) {
    cell_ = cell;
    [self setAnimationBlockingMode:NSAnimationNonblocking];
  }
  return self;
}

- (void)setCurrentProgress:(NSAnimationProgress)progress {
  [super setCurrentProgress:progress];
  [cell_ animation:self progressed:progress];
}

@end

@implementation IndeterminateProgressTimer

- (id)initWithDownloadItemCell:(DownloadItemCell*)cell {
  if ((self = [super init])) {
    cell_ = cell;
    timer_.reset([[NSTimer
        scheduledTimerWithTimeInterval:DownloadShelf::kProgressRateMs / 1000.0
                                target:self
                              selector:@selector(onTimer:)
                              userInfo:nil
                               repeats:YES] retain]);
  }
  return self;
}

- (void)invalidate {
  [timer_ invalidate];
}

- (void)onTimer:(NSTimer*)timer {
  [cell_ updateIndeterminateDownload];
}

@end
