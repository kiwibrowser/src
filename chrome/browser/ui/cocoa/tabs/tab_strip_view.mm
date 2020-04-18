// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabs/tab_strip_view.h"

#include <cmath>  // floor

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#include "base/mac/sdk_forward_declarations.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"
#import "chrome/browser/ui/cocoa/new_tab_button.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#import "chrome/browser/ui/cocoa/tabs/tab_view.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#import "ui/base/cocoa/appkit_utils.h"
#import "ui/base/cocoa/nsgraphics_context_additions.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

@implementation TabStripView

@synthesize dropArrowShown = dropArrowShown_;
@synthesize dropArrowPosition = dropArrowPosition_;
@synthesize inATabDraggingOverlayWindow = inATabDraggingOverlayWindow_;

- (id)initWithFrame:(NSRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    newTabButton_.reset([[NewTabButton alloc] initWithFrame:
        NSMakeRect(295, 0, 40, 27)]);
    [newTabButton_ setToolTip:l10n_util::GetNSString(IDS_TOOLTIP_NEW_TAB)];

    // Set lastMouseUp_ = -1000.0 so that timestamp-lastMouseUp_ is big unless
    // lastMouseUp_ has been reset.
    lastMouseUp_ = -1000.0;

    // Register to be an URL drop target.
    dropHandler_.reset([[URLDropTargetHandler alloc] initWithView:self]);

    [self setShowsDivider:NO];

    [self setWantsLayer:YES];
  }
  return self;
}

// Draw the bottom edge of the tab strip. Each tab is responsible for mimicking
// this bottom border, unless it's the selected tab.
- (void)drawBottomEdge:(NSRect)dirtyRect {
  NSWindow* window = [self window];
  const ui::ThemeProvider* themeProvider = [window themeProvider];
  if (!themeProvider)
    return;

  NSColor* strokeColor;
  if (themeProvider->HasCustomImage(IDR_THEME_TOOLBAR) ||
      themeProvider->HasCustomColor(ThemeProperties::COLOR_TOOLBAR)) {
    // First draw the toolbar bitmap, so that theme colors can shine through.
    NSRect backgroundRect = [self bounds];
    backgroundRect.size.height = [self cr_lineWidth];
    if (NSIntersectsRect(backgroundRect, dirtyRect)) {
      [self drawBackground:backgroundRect];
    }

    // Pre-MD the IDR_TOOLBAR_SHADE_TOP image would lay down a light highlight
    // which helped dark toolbars stand out from dark frames. Lay down a thin
    // highlight in MD also.
    if ([window isMainWindow]) {
      strokeColor = themeProvider->GetNSColor(
          ThemeProperties::COLOR_TOOLBAR_STROKE_THEME);
    } else {
      strokeColor = themeProvider->GetNSColor(
          ThemeProperties::COLOR_TOOLBAR_STROKE_THEME_INACTIVE);
    }
  } else {
    strokeColor =
        themeProvider->GetNSColor(ThemeProperties::COLOR_TOOLBAR_STROKE);

    // If the current theme is the system theme, and the system is in "increase
    // contrast" mode, and this is an incognito window, force the toolbar stroke
    // to be drawn in white instead of black, to make it show up better.
    if ([[self window] hasDarkTheme] && themeProvider->ShouldIncreaseContrast())
      strokeColor = [NSColor whiteColor];
  }

  if (themeProvider->ShouldIncreaseContrast())
    strokeColor = [strokeColor colorWithAlphaComponent:100];
  [strokeColor set];

  NSRect borderRect = NSMakeRect(0.0, 0.0, self.bounds.size.width,
      [self cr_lineWidth]);
  NSRectFillUsingOperation(NSIntersectionRect(dirtyRect, borderRect),
                           NSCompositeSourceOver);
}

- (void)drawRect:(NSRect)dirtyRect {
  const ui::ThemeProvider* themeProvider = [[self window] themeProvider];
  bool hasCustomThemeImage = themeProvider &&
      themeProvider->HasCustomImage(IDR_THEME_FRAME);
  BOOL supportsVibrancy = false;
  if (@available(macOS 10.10, *))
    supportsVibrancy = [self visualEffectView] != nil;
  BOOL isMainWindow = [[self window] isMainWindow];

  // If in Material Design mode, decrease the tabstrip background's translucency
  // by overlaying it with a partially-transparent gray (but only if not themed,
  // and not being used to drag tabs between browser windows). The gray is
  // somewhat opaque for Incognito mode, very opaque for non-Incognito mode, and
  // completely opaque when the window is not active.
  if (themeProvider && !hasCustomThemeImage && !inATabDraggingOverlayWindow_) {
    NSColor* theColor = nil;
    if (isMainWindow) {
      if (supportsVibrancy &&
          !themeProvider->HasCustomColor(ThemeProperties::COLOR_FRAME)) {
        theColor = themeProvider->GetNSColor(
            ThemeProperties::COLOR_FRAME_VIBRANCY_OVERLAY);
      } else if (!supportsVibrancy && themeProvider->InIncognitoMode()) {
        theColor = [NSColor colorWithSRGBRed:20 / 255.
                                       green:22 / 255.
                                        blue:24 / 255.
                                       alpha:1];
      } else {
        theColor = themeProvider->GetNSColor(ThemeProperties::COLOR_FRAME);
      }
    } else {
      theColor = themeProvider->GetNSColor(
          ThemeProperties::COLOR_FRAME_INACTIVE);
    }

    if (theColor) {
      [theColor set];
      NSRectFillUsingOperation(dirtyRect, NSCompositeSourceOver);
    }
  }

  [self drawBottomEdge:dirtyRect];

  // Draw drop-indicator arrow (if appropriate).
  // TODO(viettrungluu): this is all a stop-gap measure.
  if ([self dropArrowShown]) {
    // Programmer art: an arrow parametrized by many knobs. Note that the arrow
    // points downwards (so understand "width" and "height" accordingly).

    // How many (pixels) to inset on the top/bottom.
    const CGFloat kArrowTopInset = 1.5;
    const CGFloat kArrowBottomInset = 1;

    // What proportion of the vertical space is dedicated to the arrow tip,
    // i.e., (arrow tip height)/(amount of vertical space).
    const CGFloat kArrowTipProportion = 0.55;

    // This is a slope, i.e., (arrow tip height)/(0.5 * arrow tip width).
    const CGFloat kArrowTipSlope = 1.2;

    // What proportion of the arrow tip width is the stem, i.e., (stem
    // width)/(arrow tip width).
    const CGFloat kArrowStemProportion = 0.33;

    NSPoint arrowTipPos = [self dropArrowPosition];
    arrowTipPos.x = std::floor(arrowTipPos.x);  // Draw on the pixel.
    arrowTipPos.y += kArrowBottomInset;  // Inset on the bottom.

    // Height we have to work with (insetting on the top).
    CGFloat availableHeight =
        NSMaxY([self bounds]) - arrowTipPos.y - kArrowTopInset;
    DCHECK(availableHeight >= 5);

    // Based on the knobs above, calculate actual dimensions which we'll need
    // for drawing.
    CGFloat arrowTipHeight = kArrowTipProportion * availableHeight;
    CGFloat arrowTipWidth = 2 * arrowTipHeight / kArrowTipSlope;
    CGFloat arrowStemHeight = availableHeight - arrowTipHeight;
    CGFloat arrowStemWidth = kArrowStemProportion * arrowTipWidth;
    CGFloat arrowStemInset = (arrowTipWidth - arrowStemWidth) / 2;

    // The line width is arbitrary, but our path really should be mitered.
    NSBezierPath* arrow = [NSBezierPath bezierPath];
    [arrow setLineJoinStyle:NSMiterLineJoinStyle];
    [arrow setLineWidth:1];

    // Define the arrow's shape! We start from the tip and go clockwise.
    [arrow moveToPoint:arrowTipPos];
    [arrow relativeLineToPoint:NSMakePoint(-arrowTipWidth / 2, arrowTipHeight)];
    [arrow relativeLineToPoint:NSMakePoint(arrowStemInset, 0)];
    [arrow relativeLineToPoint:NSMakePoint(0, arrowStemHeight)];
    [arrow relativeLineToPoint:NSMakePoint(arrowStemWidth, 0)];
    [arrow relativeLineToPoint:NSMakePoint(0, -arrowStemHeight)];
    [arrow relativeLineToPoint:NSMakePoint(arrowStemInset, 0)];
    [arrow closePath];

    // Draw and fill the arrow.
    [[NSColor colorWithCalibratedWhite:0 alpha:0.67] set];
    [arrow stroke];
    [[NSColor colorWithCalibratedWhite:1 alpha:0.67] setFill];
    [arrow fill];
  }
}

// YES if a double-click in the background of the tab strip minimizes the
// window.
- (BOOL)doubleClickMinimizesWindow {
  return YES;
}

// We accept first mouse so clicks onto close/zoom/miniaturize buttons and
// title bar double-clicks are properly detected even when the window is in the
// background.
- (BOOL)acceptsFirstMouse:(NSEvent*)event {
  return YES;
}

// When displaying a modal sheet, interaction with the tabs (e.g. middle-click
// to close a tab) should be blocked. -[NSWindow sendEvent] blocks left-click,
// but not others. To prevent clicks going to subviews, absorb them here. This
// is also done in FastResizeView, but TabStripView is in the title bar, so is
// not contained in a FastResizeView.
- (NSView*)hitTest:(NSPoint)aPoint {
  if ([[self window] attachedSheet])
    return self;
  return [super hitTest:aPoint];
}

// Trap double-clicks and make them miniaturize the browser window.
- (void)mouseUp:(NSEvent*)event {
  // Bail early if double-clicks are disabled.
  if (![self doubleClickMinimizesWindow]) {
    [super mouseUp:event];
    return;
  }

  NSInteger clickCount = [event clickCount];
  NSTimeInterval timestamp = [event timestamp];

  // Double-clicks on Zoom/Close/Mininiaturize buttons shouldn't cause
  // miniaturization. For those, we miss the first click but get the second
  // (with clickCount == 2!). We thus check that we got a first click shortly
  // before (measured up-to-up) a double-click. Cocoa doesn't have a documented
  // way of getting the proper interval (= (double-click-threshold) +
  // (drag-threshold); the former is Carbon GetDblTime()/60.0 or
  // com.apple.mouse.doubleClickThreshold [undocumented]). So we hard-code
  // "short" as 0.8 seconds. (Measuring up-to-up isn't enough to properly
  // detect double-clicks, but we're actually using Cocoa for that.)
  if (clickCount == 2 && (timestamp - lastMouseUp_) < 0.8) {
    ui::WindowTitlebarReceivedDoubleClick([self window], self);
  } else {
    [super mouseUp:event];
  }

  // If clickCount is 0, the drag threshold was passed.
  lastMouseUp_ = (clickCount == 1) ? timestamp : -1000.0;
}

// (URLDropTarget protocol)
- (id<URLDropTargetController>)urlDropController {
  return controller_;
}

// (URLDropTarget protocol)
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
  return [dropHandler_ draggingEntered:sender];
}

// (URLDropTarget protocol)
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
  return [dropHandler_ draggingUpdated:sender];
}

// (URLDropTarget protocol)
- (void)draggingExited:(id<NSDraggingInfo>)sender {
  return [dropHandler_ draggingExited:sender];
}

// (URLDropTarget protocol)
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
  return [dropHandler_ performDragOperation:sender];
}

- (BOOL)accessibilityIsIgnored {
  return NO;
}

// Returns AX children (tabs and new tab button), sorted from left to right.
- (NSArray*)tabStripViewAccessibilityChildren {
  NSArray* children =
      [super accessibilityAttributeValue:NSAccessibilityChildrenAttribute];
  return [children sortedArrayUsingComparator:
      ^NSComparisonResult(id first, id second) {
          NSPoint firstPosition =
              [[first accessibilityAttributeValue:
                          NSAccessibilityPositionAttribute] pointValue];
          NSPoint secondPosition =
              [[second accessibilityAttributeValue:
                           NSAccessibilityPositionAttribute] pointValue];
          if (firstPosition.x < secondPosition.x)
            return NSOrderedAscending;
          else if (firstPosition.x > secondPosition.x)
            return NSOrderedDescending;
          else
            return NSOrderedSame;
      }];
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([attribute isEqual:NSAccessibilityRoleAttribute]) {
    return NSAccessibilityTabGroupRole;
  } else if ([attribute isEqual:NSAccessibilityChildrenAttribute]) {
    return [self tabStripViewAccessibilityChildren];
  } else if ([attribute isEqual:NSAccessibilityTabsAttribute]) {
    NSArray* children = [self tabStripViewAccessibilityChildren];
    NSIndexSet* indexes = [children indexesOfObjectsPassingTest:
        ^BOOL(id child, NSUInteger idx, BOOL* stop) {
            NSString* role = [child
                accessibilityAttributeValue:NSAccessibilityRoleAttribute];
            return [role isEqualToString:NSAccessibilityRadioButtonRole];
        }];
    return [children objectsAtIndexes:indexes];
  } else if ([attribute isEqual:NSAccessibilityContentsAttribute]) {
    return [self tabStripViewAccessibilityChildren];
  } else if ([attribute isEqual:NSAccessibilityValueAttribute]) {
    return [controller_ activeTabView];
  }

  return [super accessibilityAttributeValue:attribute];
}

- (NSArray*)accessibilityAttributeNames {
  NSMutableArray* attributes =
      [[super accessibilityAttributeNames] mutableCopy];
  [attributes addObject:NSAccessibilityTabsAttribute];
  [attributes addObject:NSAccessibilityContentsAttribute];
  [attributes addObject:NSAccessibilityValueAttribute];

  return [attributes autorelease];
}

- (ViewID)viewID {
  return VIEW_ID_TAB_STRIP;
}

- (NewTabButton*)getNewTabButton {
  return newTabButton_;
}

- (void)setNewTabButton:(NewTabButton*)button {
  newTabButton_.reset([button retain]);
}

- (NSVisualEffectView*)visualEffectView API_AVAILABLE(macos(10.10)) {
  return [[BrowserWindowController
      browserWindowControllerForWindow:[self window]] visualEffectView];
}

- (void)setController:(TabStripController*)controller {
  controller_ = controller;
  // If tearing down the browser window, there's nothing more to do.
  if (!controller_) {
    return;
  }

  // Finish configuring the NSVisualEffectView so that it matches the window's
  // theme.
  if (@available(macOS 10.10, *)) {
    NSVisualEffectView* visualEffectView = [self visualEffectView];
    const ui::ThemeProvider* themeProvider = [[self window] themeProvider];
    if (!visualEffectView || !themeProvider) {
      return;
    }

    // Themes with custom frame images don't use vibrancy. Otherwise, if
    // Incognito use Material Dark.
    if (themeProvider->HasCustomImage(IDR_THEME_FRAME) ||
        themeProvider->HasCustomColor(ThemeProperties::COLOR_FRAME)) {
      [visualEffectView setState:NSVisualEffectStateInactive];
    } else if (themeProvider->InIncognitoMode()) {
      [visualEffectView setMaterial:NSVisualEffectMaterialDark];
      [visualEffectView
          setAppearance:[NSAppearance
                            appearanceNamed:NSAppearanceNameVibrantDark]];
    }
  }
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  [self setNeedsDisplay:YES];
  [self updateVisualEffectState];
}

- (void)windowDidChangeActive {
  [self setNeedsDisplay:YES];
}

- (void)setVisualEffectsDisabledForFullscreen:(BOOL)disabled {
  visualEffectsDisabledForFullscreen_ = disabled;
  [self updateVisualEffectState];
}

- (void)updateVisualEffectState {
  if (@available(macOS 10.10, *)) {
    // Configure the NSVisualEffectView so that it does nothing if the user has
    // switched to a custom theme, or uses vibrancy if the user has switched
    // back to the default theme.
    NSVisualEffectView* visualEffectView = [self visualEffectView];
    const ui::ThemeProvider* themeProvider = [[self window] themeProvider];
    if (!visualEffectView || !themeProvider) {
      return;
    }
    if (visualEffectsDisabledForFullscreen_ ||
        themeProvider->HasCustomImage(IDR_THEME_FRAME) ||
        themeProvider->HasCustomColor(ThemeProperties::COLOR_FRAME)) {
      [visualEffectView setState:NSVisualEffectStateInactive];
    } else {
      [visualEffectView setState:NSVisualEffectStateFollowsWindowActiveState];
    }
  }
}

@end
