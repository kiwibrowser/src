// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_view.h"

#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/tabs/alert_indicator_button_cocoa.h"
#import "chrome/browser/ui/cocoa/tabs/tab_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_window_controller.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/theme_resources.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMFadeTruncatingTextFieldCell.h"
#import "ui/base/cocoa/nsgraphics_context_additions.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/cocoa/three_part_image.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"
#include "ui/strings/grit/ui_strings.h"

namespace {

// The color of the icons in dark mode theme.
const SkColor kDarkModeIconColor = SkColorSetARGB(0xFF, 0xC4, 0xC4, 0xC4);

bool IsTabStripKeyboardFocusEnabled() {
  return base::FeatureList::IsEnabled(features::kTabStripKeyboardFocus) &&
         [NSApp isFullKeyboardAccessEnabled];
}

}  // namespace

// The amount of time in seconds during which each type of glow increases, holds
// steady, and decreases, respectively.
const NSTimeInterval kHoverShowDuration = 0.2;
const NSTimeInterval kHoverHoldDuration = 0.02;
const NSTimeInterval kHoverHideDuration = 0.4;

// The default time interval in seconds between glow updates (when
// increasing/decreasing).
const NSTimeInterval kGlowUpdateInterval = 0.025;

// The intensity of the white overlay when hovering the mouse over a tab.
const CGFloat kMouseHoverWhiteValue = 1.0;
const CGFloat kMouseHoverWhiteValueIncongito = 0.3;

// This is used to judge whether the mouse has moved during rapid closure; if it
// has moved less than the threshold, we want to close the tab.
const CGFloat kRapidCloseDist = 2.5;

@interface NSView (PrivateAPI)
// Called by AppKit to check if dragging this view should move the window.
// NSButton overrides this method in the same way so dragging window buttons
// has no effect. NSView implementation returns NSZeroRect so the whole view
// area can be dragged.
- (NSRect)_opaqueRectForWindowMoveWhenInTitlebar;
@end

// This class contains the logic for drawing Material Design tab images. The
// |setTabEdgeStrokeColor| method is overridden by |TabHeavyImageMaker| to draw
// high-contrast tabs.
@interface TabImageMaker : NSObject
+ (void)drawTabLeftMaskImage;
+ (void)drawTabRightMaskImage;
+ (void)drawTabLeftEdgeImage;
+ (void)drawTabMiddleEdgeImage;
+ (void)drawTabRightEdgeImage;
+ (void)setTabEdgeStrokeColor;
@end

@interface TabHeavyImageMaker : TabImageMaker
+ (void)setTabEdgeStrokeColor;
@end

@interface TabHeavyInvertedImageMaker : TabImageMaker
+ (void)setTabEdgeStrokeColor;
@end

extern NSString* const _Nonnull NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification;

namespace {

enum StrokeType {
  STROKE_NORMAL,
  STROKE_HEAVY,
  STROKE_HEAVY_INVERTED,
};

Class drawingClassForStrokeType(StrokeType stroke_type) {
  switch (stroke_type) {
    case STROKE_NORMAL:
      return [TabImageMaker class];
    case STROKE_HEAVY:
      return [TabHeavyImageMaker class];
    case STROKE_HEAVY_INVERTED:
      return [TabHeavyInvertedImageMaker class];
  }
}

NSImage* imageForResourceID(int resource_id, StrokeType stroke_type) {
  CGFloat imageWidth = resource_id == IDR_TAB_ACTIVE_CENTER ? 1 : 18;
  SEL theSelector = 0;
  switch (resource_id) {
    case IDR_TAB_ACTIVE_LEFT:
      theSelector = @selector(drawTabLeftEdgeImage);
      break;

    case IDR_TAB_ACTIVE_CENTER:
      theSelector = @selector(drawTabMiddleEdgeImage);
      break;

    case IDR_TAB_ACTIVE_RIGHT:
      theSelector = @selector(drawTabRightEdgeImage);
      break;

    case IDR_TAB_ALPHA_LEFT:
      theSelector = @selector(drawTabLeftMaskImage);
      break;

    case IDR_TAB_ALPHA_RIGHT:
      theSelector = @selector(drawTabRightMaskImage);
      break;
  }
  DCHECK(theSelector);

  Class makerClass = drawingClassForStrokeType(stroke_type);
  base::scoped_nsobject<NSCustomImageRep> imageRep([[NSCustomImageRep alloc]
      initWithDrawSelector:theSelector
                  delegate:makerClass]);

  NSImage* newTabButtonImage =
      [[[NSImage alloc] initWithSize:NSMakeSize(imageWidth, 29)] autorelease];
  [newTabButtonImage setCacheMode:NSImageCacheAlways];
  [newTabButtonImage addRepresentation:imageRep];

  return newTabButtonImage;
}

ui::ThreePartImage& GetMaskImage() {
  CR_DEFINE_STATIC_LOCAL(
      ui::ThreePartImage, mask,
      (imageForResourceID(IDR_TAB_ALPHA_LEFT, STROKE_NORMAL), nullptr,
       imageForResourceID(IDR_TAB_ALPHA_RIGHT, STROKE_NORMAL)));

  return mask;
}

ui::ThreePartImage& GetStrokeImage(bool active, StrokeType stroke_type) {
  CR_DEFINE_STATIC_LOCAL(
      ui::ThreePartImage, stroke,
      (imageForResourceID(IDR_TAB_ACTIVE_LEFT, STROKE_NORMAL),
       imageForResourceID(IDR_TAB_ACTIVE_CENTER, STROKE_NORMAL),
       imageForResourceID(IDR_TAB_ACTIVE_RIGHT, STROKE_NORMAL)));
  CR_DEFINE_STATIC_LOCAL(
      ui::ThreePartImage, heavyStroke,
      (imageForResourceID(IDR_TAB_ACTIVE_LEFT, STROKE_HEAVY),
       imageForResourceID(IDR_TAB_ACTIVE_CENTER, STROKE_HEAVY),
       imageForResourceID(IDR_TAB_ACTIVE_RIGHT, STROKE_HEAVY)));
  CR_DEFINE_STATIC_LOCAL(
      ui::ThreePartImage, heavyInvertedStroke,
      (imageForResourceID(IDR_TAB_ACTIVE_LEFT, STROKE_HEAVY_INVERTED),
       imageForResourceID(IDR_TAB_ACTIVE_CENTER, STROKE_HEAVY_INVERTED),
       imageForResourceID(IDR_TAB_ACTIVE_RIGHT, STROKE_HEAVY_INVERTED)));

  switch (stroke_type) {
    case STROKE_NORMAL:
      return stroke;
    case STROKE_HEAVY:
      return heavyStroke;
    case STROKE_HEAVY_INVERTED:
      return heavyInvertedStroke;
  }
}

CGFloat LineWidthFromContext(CGContextRef context) {
  CGRect unitRect = CGRectMake(0.0, 0.0, 1.0, 1.0);
  CGRect deviceRect = CGContextConvertRectToDeviceSpace(context, unitRect);
  return 1.0 / deviceRect.size.height;
}

}  // namespace

@interface TabView(Private)

- (void)resetLastGlowUpdateTime;
- (NSTimeInterval)timeElapsedSinceLastGlowUpdate;
- (void)adjustGlowValue;

@end  // TabView(Private)

@implementation TabView

@synthesize state = state_;
@synthesize hoverAlpha = hoverAlpha_;
@synthesize closing = closing_;

+ (CGFloat)maskImageFillHeight {
  // Return the height of the "mask on" part of the mask bitmap.
  return [TabController defaultTabHeight] - 1;
}

- (id)initWithFrame:(NSRect)frame
         controller:(TabController*)controller
        closeButton:(HoverCloseButton*)closeButton {
  self = [super initWithFrame:frame];
  if (self) {
    controller_ = controller;
    closeButton_ = closeButton;
    [self addSubview:closeButton_];

    // Make a text field for the title, but don't add it as a subview.
    // We will use the cell to draw the text directly into our layer,
    // so that we can get font smoothing enabled.
    titleView_.reset([[NSTextField alloc] init]);
    [titleView_ setAutoresizingMask:NSViewWidthSizable];
    base::scoped_nsobject<GTMFadeTruncatingTextFieldCell> labelCell(
        [[GTMFadeTruncatingTextFieldCell alloc] initTextCell:@"Label"]);
    [labelCell setControlSize:NSSmallControlSize];
    if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout())
      [labelCell setAlignment:NSRightTextAlignment];
    [titleView_ setCell:labelCell];
    titleViewCell_ = labelCell;

    [self setWantsLayer:YES];  // -drawFill: needs a layer.

    if (@available(macOS 10.10, *)) {
      NSNotificationCenter* center =
          [[NSWorkspace sharedWorkspace] notificationCenter];
      [center
          addObserver:self
             selector:@selector(accessibilityOptionsDidChange:)
                 name:
                     NSWorkspaceAccessibilityDisplayOptionsDidChangeNotification
               object:nil];
    }
  }
  return self;
}

- (void)dealloc {
  // Cancel any delayed requests that may still be pending (drags or hover).
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  if (@available(macOS 10.10, *)) {
    NSNotificationCenter* center =
        [[NSWorkspace sharedWorkspace] notificationCenter];
    [center removeObserver:self];
  }
  [super dealloc];
}

// Called by AppKit to check if dragging this view should move the window.
// NSButton overrides this method in the same way so dragging window buttons
// has no effect.
- (NSRect)_opaqueRectForWindowMoveWhenInTitlebar {
  return [self bounds];
}

// Called to obtain the context menu for when the user hits the right mouse
// button (or control-clicks). (Note that -rightMouseDown: is *not* called for
// control-click.)
- (NSMenu*)menu {
  if ([self isClosing])
    return nil;

  return [controller_ menu];
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldBoundsSize {
  [super resizeSubviewsWithOldSize:oldBoundsSize];
  // Called when our view is resized. If it gets too small, start by hiding
  // the close button and only show it if tab is selected. Eventually, hide the
  // icon as well.
  [controller_ updateVisibility];
}

// Overridden so that mouse clicks come to this view (the parent of the
// hierarchy) first. We want to handle clicks and drags in this class and
// leave the background button for display purposes only.
- (BOOL)acceptsFirstMouse:(NSEvent*)theEvent {
  return YES;
}

- (void)mouseEntered:(NSEvent*)theEvent {
  isMouseInside_ = YES;
  [self resetLastGlowUpdateTime];
  [self adjustGlowValue];
}

- (void)mouseMoved:(NSEvent*)theEvent {
  if (state_ == NSOffState) {
    hoverPoint_ = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    [self setNeedsDisplay:YES];
  }
}

- (void)mouseExited:(NSEvent*)theEvent {
  isMouseInside_ = NO;
  hoverHoldEndTime_ =
      [NSDate timeIntervalSinceReferenceDate] + kHoverHoldDuration;
  [self resetLastGlowUpdateTime];
  [self adjustGlowValue];
}

- (void)setTrackingEnabled:(BOOL)enabled {
  if (![closeButton_ isHidden]) {
    [closeButton_ setTrackingEnabled:enabled];
  }
}

// Determines which view a click in our frame actually hit. It's either this
// view or one of the child buttons.
- (NSView*)hitTest:(NSPoint)aPoint {
  NSView* const defaultHitTestResult = [super hitTest:aPoint];
  if ([defaultHitTestResult isKindOfClass:[NSButton class]])
    return defaultHitTestResult;

  NSPoint viewPoint = [self convertPoint:aPoint fromView:[self superview]];
  NSRect maskRect = [self bounds];
  maskRect.size.height = [TabView maskImageFillHeight];
  return GetMaskImage().HitTest(viewPoint, maskRect) ? self : nil;
}

// Handle clicks and drags in this button. We get here because we have
// overridden acceptsFirstMouse: and the click is within our bounds.
- (void)mouseDown:(NSEvent*)theEvent {
  if ([self isClosing])
    return;

  // Record the point at which this event happened. This is used by other mouse
  // events that are dispatched from |-maybeStartDrag::|.
  mouseDownPoint_ = [theEvent locationInWindow];

  // Record the state of the close button here, because selecting the tab will
  // unhide it.
  BOOL closeButtonActive = ![closeButton_ isHidden];

  // During the tab closure animation (in particular, during rapid tab closure),
  // we may get incorrectly hit with a mouse down. If it should have gone to the
  // close button, we send it there -- it should then track the mouse, so we
  // don't have to worry about mouse ups.
  if (closeButtonActive && [controller_ inRapidClosureMode]) {
    NSPoint hitLocation = [[self superview] convertPoint:mouseDownPoint_
                                                fromView:nil];
    if ([self hitTest:hitLocation] == closeButton_) {
      [closeButton_ mouseDown:theEvent];
      return;
    }
  }

  // If the tab gets torn off, the tab controller will be removed from the tab
  // strip and then deallocated. This will also result in *us* being
  // deallocated. Both these are bad, so we prevent this by retaining the
  // controller.
  base::scoped_nsobject<TabController> controller([controller_ retain]);

  // Try to initiate a drag. This will spin a custom event loop and may
  // dispatch other mouse events.
  [controller_ maybeStartDrag:theEvent forTab:controller];

  // The custom loop has ended, so clear the point.
  mouseDownPoint_ = NSZeroPoint;
}

- (void)mouseUp:(NSEvent*)theEvent {
  // Check for rapid tab closure.
  if ([theEvent type] == NSLeftMouseUp) {
    NSPoint upLocation = [theEvent locationInWindow];
    CGFloat dx = upLocation.x - mouseDownPoint_.x;
    CGFloat dy = upLocation.y - mouseDownPoint_.y;

    // During rapid tab closure (mashing tab close buttons), we may get hit
    // with a mouse down. As long as the mouse up is over the close button,
    // and the mouse hasn't moved too much, we close the tab.
    if (![closeButton_ isHidden] &&
        (dx*dx + dy*dy) <= kRapidCloseDist*kRapidCloseDist &&
        [controller_ inRapidClosureMode]) {
      NSPoint hitLocation =
          [[self superview] convertPoint:[theEvent locationInWindow]
                                fromView:nil];
      if ([self hitTest:hitLocation] == closeButton_) {
        [controller_ closeTab:self];
        return;
      }
    }
  }

  // Except in the rapid tab closure case, mouseDown: triggers a nested run loop
  // that swallows the mouseUp: event. There's a bug in AppKit that sends
  // mouseUp: callbacks to inappropriate views, so it's doubly important that
  // this method doesn't do anything. https://crbug.com/511095.
  [super mouseUp:theEvent];
}

- (void)otherMouseUp:(NSEvent*)theEvent {
  if ([self isClosing])
    return;

  // Support middle-click-to-close.
  if ([theEvent buttonNumber] == 2) {
    // |-hitTest:| takes a location in the superview's coordinates.
    NSPoint upLocation =
        [[self superview] convertPoint:[theEvent locationInWindow]
                              fromView:nil];
    // If the mouse up occurred in our view or over the close button, then
    // close.
    if ([self hitTest:upLocation])
      [controller_ closeTab:self];
  }
}

// Returns the color used to draw the background of a tab. |selected| selects
// between the foreground and background tabs.
- (NSColor*)backgroundColorForSelected:(bool)selected {
  const ui::ThemeProvider* themeProvider = [[self window] themeProvider];
  if (!themeProvider)
    return [[self window] backgroundColor];

  int bitmapResources[2][2] = {
    // Background window.
    {
      IDR_THEME_TAB_BACKGROUND_INACTIVE,  // Background tab.
      IDR_THEME_TOOLBAR_INACTIVE,         // Active tab.
    },
    // Currently focused window.
    {
      IDR_THEME_TAB_BACKGROUND,  // Background tab.
      IDR_THEME_TOOLBAR,         // Active tab.
    },
  };

  // Themes don't have an inactive image so only look for one if there's no
  // theme.
  bool active =
      [[self window] isMainWindow] || !themeProvider->UsingSystemTheme();
  return themeProvider->GetNSImageColorNamed(bitmapResources[active][selected]);
}

// Draws the tab background.
- (void)drawFill:(NSRect)dirtyRect {
  gfx::ScopedNSGraphicsContextSaveGState scopedGState;
  NSRect bounds = [self bounds];

  NSRect clippingRect = bounds;
  clippingRect.size.height = [TabView maskImageFillHeight];
  if (state_ != NSOnState) {
    // Background tabs should not paint over the tab strip separator, which is
    // two pixels high in both lodpi and hidpi, and one pixel high in MD.
    CGFloat tabStripSeparatorLineWidth = [self cr_lineWidth];
    clippingRect.origin.y = tabStripSeparatorLineWidth;
    clippingRect.size.height -= tabStripSeparatorLineWidth;
  }
  NSRectClip(clippingRect);

  NSPoint position = [[self window]
      themeImagePositionForAlignment:THEME_IMAGE_ALIGN_WITH_TAB_STRIP];
  [[NSGraphicsContext currentContext] cr_setPatternPhase:position forView:self];

  [[self backgroundColorForSelected:(state_ != NSOffState)] set];
  NSRectFill(dirtyRect);

  if (state_ == NSOffState)
    [self drawGlow:dirtyRect];

  // If we filled outside the middle rect, we need to erase what we filled
  // outside the tab's shape.
  // This only works if we are drawing to our own backing layer.
  if (!NSContainsRect(GetMaskImage().GetMiddleRect(bounds), dirtyRect)) {
    DCHECK([self layer]);
    GetMaskImage().DrawInRect(bounds, NSCompositeDestinationIn, 1.0);
  }
}

// Draw the glow for hover and the overlay for alerts.
- (void)drawGlow:(NSRect)dirtyRect {
  NSGraphicsContext* context = [NSGraphicsContext currentContext];
  CGContextRef cgContext = static_cast<CGContextRef>([context graphicsPort]);

  CGFloat hoverAlpha = [self hoverAlpha];
  if (hoverAlpha > 0) {
    CGContextBeginTransparencyLayer(cgContext, 0);

    // The hover glow brings up the overlay's opacity at most 50%.
    CGFloat backgroundAlpha = 0.5 * hoverAlpha;
    CGContextSetAlpha(cgContext, backgroundAlpha);

    [[self backgroundColorForSelected:YES] set];
    NSRectFill(dirtyRect);

    // ui::ThemeProvider::HasCustomImage is true only if the theme provides the
    // image. However, even if the theme doesn't provide a tab background, the
    // theme machinery will make one if given a frame image. See
    // BrowserThemePack::GenerateTabBackgroundImages for details.
    const ui::ThemeProvider* themeProvider = [[self window] themeProvider];
    BOOL hasCustomTheme = themeProvider &&
        (themeProvider->HasCustomImage(IDR_THEME_TAB_BACKGROUND) ||
         themeProvider->HasCustomImage(IDR_THEME_FRAME));
    // Draw a mouse hover gradient for the default themes.
    if (hoverAlpha > 0) {
      if (themeProvider && !hasCustomTheme) {
        CGFloat whiteValue = kMouseHoverWhiteValue;
        if (themeProvider && themeProvider->InIncognitoMode()) {
          whiteValue = kMouseHoverWhiteValueIncongito;
        }
        base::scoped_nsobject<NSGradient> glow([NSGradient alloc]);
        [glow initWithStartingColor:[NSColor colorWithCalibratedWhite:whiteValue
                                        alpha:1.0 * hoverAlpha]
                        endingColor:[NSColor colorWithCalibratedWhite:whiteValue
                                                                alpha:0.0]];
        NSRect rect = [self bounds];
        NSPoint point = hoverPoint_;
        point.y = NSHeight(rect);
        [glow drawFromCenter:point
                      radius:0.0
                    toCenter:point
                      radius:NSWidth(rect) / 3.0
                     options:NSGradientDrawsBeforeStartingLocation];
      }
    }

    CGContextEndTransparencyLayer(cgContext);
  }
}

// Draws the tab outline.
- (void)drawStroke:(NSRect)dirtyRect {
  // In MD, the tab stroke is always opaque.
  CGFloat alpha = 1;
  NSRect bounds = [self bounds];
  // In Material Design the tab strip separator is always 1 pixel high -
  // add a clip rect to avoid drawing the tab edge over it.
  NSRect clipRect = bounds;
  clipRect.origin.y += [self cr_lineWidth];
  NSRectClip(clipRect);
  const ui::ThemeProvider* provider = [[self window] themeProvider];
  StrokeType stroke_type = STROKE_NORMAL;
  if (provider && provider->ShouldIncreaseContrast()) {
    stroke_type =
        [[self window] hasDarkTheme] ? STROKE_HEAVY_INVERTED : STROKE_HEAVY;
  }
  GetStrokeImage(state_ == NSOnState, stroke_type)
      .DrawInRect(bounds, NSCompositeSourceOver, alpha);
}

- (void)drawRect:(NSRect)dirtyRect {
  [self drawFill:dirtyRect];
  [self drawStroke:dirtyRect];

  // We draw the title string directly instead of using a NSTextField subview.
  // This is so that we can get font smoothing to work on earlier OS, and even
  // when the tab background is a pattern image (when using themes).
  if (![titleView_ isHidden]) {
    gfx::ScopedNSGraphicsContextSaveGState scopedGState;
    NSGraphicsContext* context = [NSGraphicsContext currentContext];
    CGContextRef cgContext = static_cast<CGContextRef>([context graphicsPort]);
    CGContextSetShouldSmoothFonts(cgContext, true);
    [[titleView_ cell] drawWithFrame:[titleView_ frame] inView:self];
  }
}

- (void)setFrameOrigin:(NSPoint)origin {
  // The background color depends on the view's vertical position.
  if (NSMinY([self frame]) != origin.y)
    [self setNeedsDisplay:YES];
  [super setFrameOrigin:origin];
}

- (void)setToolTipText:(NSString*)string {
  toolTipText_.reset([string copy]);
}

- (NSString*)toolTipText {
  return toolTipText_;
}

- (void)viewDidMoveToWindow {
  [super viewDidMoveToWindow];
  if ([self window]) {
    [controller_ updateTitleColor];

    // The new window may have different main window status.
    // This happens when the view is moved into a TabWindowOverlayWindow for
    // tab dragging.
    [self windowDidChangeActive];
  }
}

- (BOOL)isActiveTab {
  return [controller_ active];
}

- (NSString*)title {
  return [titleView_ stringValue];
}

- (void)setTitle:(NSString*)title {
  if ([title isEqualToString:[titleView_ stringValue]])
    return;

  [titleView_ setStringValue:title];
  [closeButton_ setAccessibilityTitle:title];

  base::string16 title16 = base::SysNSStringToUTF16(title);
  bool isRTL = base::i18n::GetFirstStrongCharacterDirection(title16) ==
               base::i18n::RIGHT_TO_LEFT;
  titleViewCell_.truncateMode = isRTL ? GTMFadeTruncatingHead
                                      : GTMFadeTruncatingTail;

  [self setNeedsDisplayInRect:[titleView_ frame]];
}

- (NSRect)titleFrame {
  return [titleView_ frame];
}

- (void)setTitleFrame:(NSRect)titleFrame {
  NSRect oldTitleFrame = [titleView_ frame];
  if (NSEqualRects(titleFrame, oldTitleFrame))
    return;
  [titleView_ setFrame:titleFrame];
  [self setNeedsDisplayInRect:NSUnionRect(titleFrame, oldTitleFrame)];
}

- (NSColor*)titleColor {
  return [titleView_ textColor];
}

- (void)setTitleColor:(NSColor*)titleColor {
  if ([titleColor isEqual:[titleView_ textColor]])
    return;
  [titleView_ setTextColor:titleColor];
  [self setNeedsDisplayInRect:[titleView_ frame]];
  [self updateAppearance];
}

- (BOOL)titleHidden {
  return [titleView_ isHidden];
}

- (void)setTitleHidden:(BOOL)titleHidden {
  if (titleHidden == [titleView_ isHidden])
    return;
  [titleView_ setHidden:titleHidden];
  [self setNeedsDisplayInRect:[titleView_ frame]];
}

- (SkColor)iconColor {
  if ([[self window] hasDarkTheme])
    return kDarkModeIconColor;

  const ui::ThemeProvider* themeProvider = [[self window] themeProvider];
  if (themeProvider) {
    bool useActiveTabTextColor = [self isActiveTab];

    const SkColor titleColor =
        useActiveTabTextColor
            ? themeProvider->GetColor(ThemeProperties::COLOR_TAB_TEXT)
            : themeProvider->GetColor(
                  ThemeProperties::COLOR_BACKGROUND_TAB_TEXT);
    return SkColorSetA(titleColor, 0xA0);
  }

  return tabs::kDefaultTabTextColor;
}

- (SkColor)alertIndicatorColorForState:(TabAlertState)state {
  // If theme provider is not yet available, return the default button
  // color.
  const ui::ThemeProvider* themeProvider = [[self window] themeProvider];
  if (!themeProvider)
    return [self iconColor];

  switch (state) {
    case TabAlertState::MEDIA_RECORDING:
      return themeProvider->GetColor(
          ThemeProperties::COLOR_TAB_ALERT_RECORDING);
    case TabAlertState::PIP_PLAYING:
      return themeProvider->GetColor(ThemeProperties::COLOR_TAB_PIP_PLAYING);
    case TabAlertState::AUDIO_PLAYING:
    case TabAlertState::AUDIO_MUTING:
    case TabAlertState::TAB_CAPTURING:
    case TabAlertState::BLUETOOTH_CONNECTED:
    case TabAlertState::USB_CONNECTED:
    case TabAlertState::NONE:
      return [self iconColor];
    default:
      NOTREACHED();
      return [self iconColor];
  }
}

- (void)accessibilityOptionsDidChange:(id)ignored {
  [self updateAppearance];
  [self setNeedsDisplay:YES];
}

- (void)updateAppearance {
  CGFloat fontSize = [titleViewCell_ font].pointSize;
  const ui::ThemeProvider* provider = [[self window] themeProvider];
  if (provider && provider->ShouldIncreaseContrast() && state_ == NSOnState) {
    [titleViewCell_ setFont:[NSFont boldSystemFontOfSize:fontSize]];
  } else {
    [titleViewCell_ setFont:[NSFont systemFontOfSize:fontSize]];
  }

  [closeButton_ setIconColor:[self iconColor]];
}

- (void)setState:(NSCellStateValue)state {
  if (state_ == state)
    return;
  state_ = state;
  [self updateAppearance];
  [self setNeedsDisplay:YES];
}

- (void)setClosing:(BOOL)closing {
  closing_ = closing;  // Safe because the property is nonatomic.
  // When closing, ensure clicks to the close button go nowhere.
  if (closing) {
    [closeButton_ setTarget:nil];
    [closeButton_ setAction:nil];
  }
}

- (int)widthOfLargestSelectableRegion {
  // Assume the entire region to the left of the alert indicator and/or close
  // buttons is available for click-to-select.  If neither are visible, the
  // entire tab region is available.
  AlertIndicatorButton* const indicator = [controller_ alertIndicatorButton];
  const int indicatorLeft = (!indicator || [indicator isHidden]) ?
      NSWidth([self frame]) : NSMinX([indicator frame]);
  const int closeButtonLeft = (!closeButton_ || [closeButton_ isHidden])
                                  ? NSWidth([self frame])
                                  : NSMinX([closeButton_ frame]);
  return std::min(indicatorLeft, closeButtonLeft);
}

- (BOOL)accessibilityIsIgnored {
  return NO;
}

- (NSArray*)accessibilityActionNames {
  NSArray* parentActions = [super accessibilityActionNames];

  return [parentActions arrayByAddingObject:NSAccessibilityPressAction];
}

- (NSArray*)accessibilityAttributeNames {
  NSMutableArray* attributes =
      [[super accessibilityAttributeNames] mutableCopy];
  [attributes addObject:NSAccessibilityTitleAttribute];
  [attributes addObject:NSAccessibilityEnabledAttribute];
  [attributes addObject:NSAccessibilityValueAttribute];

  return [attributes autorelease];
}

- (BOOL)accessibilityIsAttributeSettable:(NSString*)attribute {
  if ([attribute isEqual:NSAccessibilityTitleAttribute])
    return NO;

  if ([attribute isEqual:NSAccessibilityEnabledAttribute])
    return NO;

  if ([attribute isEqual:NSAccessibilityValueAttribute])
    return YES;

  return [super accessibilityIsAttributeSettable:attribute];
}

- (void)accessibilityPerformAction:(NSString*)action {
  if ([action isEqual:NSAccessibilityPressAction] &&
      [[controller_ target] respondsToSelector:[controller_ action]]) {
    [[controller_ target] performSelector:[controller_ action]
        withObject:self];
    NSAccessibilityPostNotification(self,
                                    NSAccessibilityValueChangedNotification);
  } else {
    [super accessibilityPerformAction:action];
  }
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([attribute isEqual:NSAccessibilityRoleAttribute])
    return NSAccessibilityRadioButtonRole;
  if ([attribute isEqual:NSAccessibilityRoleDescriptionAttribute])
    return l10n_util::GetNSStringWithFixup(IDS_ACCNAME_TAB_ROLE_DESCRIPTION);
  if ([attribute isEqual:NSAccessibilityTitleAttribute])
    return [controller_ accessibilityTitle];
  if ([attribute isEqual:NSAccessibilityValueAttribute])
    return [NSNumber numberWithInt:[controller_ selected]];
  if ([attribute isEqual:NSAccessibilityEnabledAttribute])
    return [NSNumber numberWithBool:YES];

  return [super accessibilityAttributeValue:attribute];
}

- (ViewID)viewID {
  return VIEW_ID_TAB;
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  [self setNeedsDisplay:YES];
}

- (void)windowDidChangeActive {
  [self setNeedsDisplay:YES];
}

- (BOOL)acceptsFirstResponder {
  return IsTabStripKeyboardFocusEnabled() ? YES : NO;
}

- (BOOL)becomeFirstResponder {
  return IsTabStripKeyboardFocusEnabled() ? YES : NO;
}

- (void)drawFocusRingMask {
  if (!IsTabStripKeyboardFocusEnabled())
    return;
  if ([titleView_ isHidden])
    NSRectFill([self bounds]);
  else
    NSRectFill([titleView_ frame]);
}

- (NSRect)focusRingMaskBounds {
  return [self bounds];
}

@end  // @implementation TabView

@implementation TabView (TabControllerInterface)

- (void)setController:(TabController*)controller {
  controller_ = controller;
}

@end  // @implementation TabView (TabControllerInterface)

@implementation TabView(Private)

- (void)resetLastGlowUpdateTime {
  lastGlowUpdate_ = [NSDate timeIntervalSinceReferenceDate];
}

- (NSTimeInterval)timeElapsedSinceLastGlowUpdate {
  return [NSDate timeIntervalSinceReferenceDate] - lastGlowUpdate_;
}

- (void)adjustGlowValue {
  // A time interval long enough to represent no update.
  const NSTimeInterval kNoUpdate = 1000000;

  // Time until next update for either glow.
  NSTimeInterval nextUpdate = kNoUpdate;

  NSTimeInterval elapsed = [self timeElapsedSinceLastGlowUpdate];
  NSTimeInterval currentTime = [NSDate timeIntervalSinceReferenceDate];

  // TODO(viettrungluu): <http://crbug.com/30617> -- split off the stuff below
  // into a pure function and add a unit test.

  CGFloat hoverAlpha = [self hoverAlpha];
  if (isMouseInside_) {
    // Increase hover glow until it's 1.
    if (hoverAlpha < 1) {
      hoverAlpha = MIN(hoverAlpha + elapsed / kHoverShowDuration, 1);
      [self setHoverAlpha:hoverAlpha];
      nextUpdate = MIN(kGlowUpdateInterval, nextUpdate);
    }  // Else already 1 (no update needed).
  } else {
    if (currentTime >= hoverHoldEndTime_) {
      // No longer holding, so decrease hover glow until it's 0.
      if (hoverAlpha > 0) {
        hoverAlpha = MAX(hoverAlpha - elapsed / kHoverHideDuration, 0);
        [self setHoverAlpha:hoverAlpha];
        nextUpdate = MIN(kGlowUpdateInterval, nextUpdate);
      }  // Else already 0 (no update needed).
    } else {
      // Schedule update for end of hold time.
      nextUpdate = MIN(hoverHoldEndTime_ - currentTime, nextUpdate);
    }
  }

  if (nextUpdate < kNoUpdate)
    [self performSelector:_cmd withObject:nil afterDelay:nextUpdate];

  [self resetLastGlowUpdateTime];
  [self setNeedsDisplay:YES];
}

@end  // @implementation TabView(Private)

@implementation TabImageMaker

+ (NSBezierPath*)tabLeftEdgeBezierPathForContext:(CGContextRef)context {
  NSBezierPath* bezierPath = [NSBezierPath bezierPath];

  [bezierPath moveToPoint:NSMakePoint(-2, 0)];
  [bezierPath curveToPoint:NSMakePoint(2.5, 2)
             controlPoint1:NSMakePoint(1.805, -0.38)
             controlPoint2:NSMakePoint(2.17, 1.415)];

  [bezierPath lineToPoint:NSMakePoint(14, 27)];
  [bezierPath curveToPoint:NSMakePoint(16, 29)
             controlPoint1:NSMakePoint(14.25, 27.25)
             controlPoint2:NSMakePoint(14.747467, 29.118899)];

  [bezierPath lineToPoint:NSMakePoint(18, 29)];

  if (!context) {
    return bezierPath;
  }

  // The line width is always 1px.
  CGFloat lineWidth = LineWidthFromContext(context);
  [bezierPath setLineWidth:lineWidth];

  // Screen pixels lay between integral coordinates in user space. If you draw
  // a line from (16, 29) to (18, 29), Core Graphics maps that line to the
  // pixels that lay along y=28.5. In order to achieve a line that appears to
  // along y=29, CG will perform dithering. To get a crisp line, you have to
  // specify y=28.5. Translating the bezier path by the 1-pixel line width
  // creates the crisp line we want.
  // On a Retina display, there are pixels at y=28.25 and y=28.75, so
  // translating the path down by one line width lights up the pixels at 28.75
  // and leaves a gap along y=28.25. To fix this for the general case we'll
  // translate up from 28 by one line width.
  NSAffineTransform* translationTransform = [NSAffineTransform transform];
  [translationTransform translateXBy:0 yBy:-1 + lineWidth / 2.];
  [bezierPath transformUsingAffineTransform:translationTransform];

  return bezierPath;
}

+ (void)setTabEdgeStrokeColor {
  static NSColor* strokeColor =
      [skia::SkColorToSRGBNSColor(SkColorSetARGB(76, 0, 0, 0)) retain];
  [strokeColor set];
}

+ (void)drawTabLeftEdgeImage {
  CGContextRef context = static_cast<CGContextRef>(
      [[NSGraphicsContext currentContext] graphicsPort]);

  [self setTabEdgeStrokeColor];
  [[self tabLeftEdgeBezierPathForContext:context] stroke];
}

+ (void)drawTabMiddleEdgeImage {
  NSBezierPath* middleEdgePath = [NSBezierPath bezierPath];
  [middleEdgePath moveToPoint:NSMakePoint(0, 29)];
  [middleEdgePath lineToPoint:NSMakePoint(1, 29)];
  [middleEdgePath setLineCapStyle:NSSquareLineCapStyle];

  CGContextRef context = static_cast<CGContextRef>(
      [[NSGraphicsContext currentContext] graphicsPort]);
  CGFloat lineWidth = LineWidthFromContext(context);

  // Line width is always 1px.
  [middleEdgePath setLineWidth:lineWidth];

  // Align to device pixels.
  NSAffineTransform* translationTransform = [NSAffineTransform transform];
  [translationTransform translateXBy:0 yBy:-1 + lineWidth / 2.];
  [middleEdgePath transformUsingAffineTransform:translationTransform];

  [self setTabEdgeStrokeColor];
  [middleEdgePath stroke];
}

+ (void)drawTabRightEdgeImage {
  CGContextRef context = static_cast<CGContextRef>(
      [[NSGraphicsContext currentContext] graphicsPort]);

  NSBezierPath* leftEdgePath = [self tabLeftEdgeBezierPathForContext:context];

  // Draw the right edge path by flipping the left edge path vertically.
  NSAffineTransform* transform = [NSAffineTransform transform];
  [transform scaleXBy:-1 yBy:1];
  [transform translateXBy:-18 yBy:0];
  [leftEdgePath transformUsingAffineTransform:transform];

  [self setTabEdgeStrokeColor];
  [leftEdgePath stroke];
}

+ (NSBezierPath*)tabLeftMaskBezierPath {
  NSBezierPath* bezierPath = [self tabLeftEdgeBezierPathForContext:nullptr];

  // Box in the open edges.
  [bezierPath lineToPoint:NSMakePoint(18, 0)];
  [bezierPath lineToPoint:NSMakePoint(0, 0)];

  [bezierPath closePath];

  return bezierPath;
}

+ (void)drawTabLeftMaskImage {
  NSBezierPath* bezierPath = [self tabLeftMaskBezierPath];
  NSAffineTransform* translationTransform = [NSAffineTransform transform];
  [translationTransform translateXBy:0.5 yBy:-0.25];
  [bezierPath transformUsingAffineTransform:translationTransform];

  [[NSColor whiteColor] set];
  [bezierPath fill];
}

+ (void)drawTabRightMaskImage {
  // Create the right mask image by flipping the left mask path along the
  // vettical axis.
  NSBezierPath* bezierPath = [self tabLeftMaskBezierPath];
  NSAffineTransform* transform = [NSAffineTransform transform];
  [transform scaleXBy:-1 yBy:1];
  [transform translateXBy:-17.5 yBy:-0.25];
  [bezierPath transformUsingAffineTransform:transform];

  [[NSColor whiteColor] set];
  [bezierPath fill];
}

@end

@implementation TabHeavyImageMaker

// For "Increase Contrast" mode, use flat black instead of semitransparent black
// for the tab edge stroke.
+ (void)setTabEdgeStrokeColor {
  static NSColor* heavyStrokeColor =
      [skia::SkColorToSRGBNSColor(SK_ColorBLACK) retain];
  [heavyStrokeColor set];
}

@end

@implementation TabHeavyInvertedImageMaker

// For "Increase Contrast" mode, when using a dark theme, the stroke should be
// drawn in flat white instead of flat black. There is normally no need to
// special-case this since the lower-contrast border is equally visible in light
// or dark themes.
+ (void)setTabEdgeStrokeColor {
  static NSColor* heavyStrokeColor =
      [skia::SkColorToSRGBNSColor(SK_ColorWHITE) retain];
  [heavyStrokeColor set];
}

@end
