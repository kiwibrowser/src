// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_toolbar_view.h"

#include "chrome/browser/search/search.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_constants.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "skia/ext/skia_utils_mac.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas_skia_paint.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

@interface BookmarkBarToolbarView (Private)
- (void)drawAsDetachedBubble:(NSRect)dirtyRect;
@end

@implementation BookmarkBarToolbarView

- (void)setController:(id<BookmarkBarToolbarViewController>)controller {
  controller_ = controller;
}

- (BOOL)isOpaque {
  // -drawRect: calls -drawAsDetachedBubble: or -drawBackground:, both of which
  // fill the dirty rect with an opaque color.
  return YES;
}

- (void)resetCursorRects {
  NSCursor *arrow = [NSCursor arrowCursor];
  [self addCursorRect:[self visibleRect] cursor:arrow];
  [arrow setOnMouseEntered:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
  if ([controller_ isInState:BookmarkBar::DETACHED] ||
      [controller_ isAnimatingToState:BookmarkBar::DETACHED] ||
      [controller_ isAnimatingFromState:BookmarkBar::DETACHED]) {
    [self drawAsDetachedBubble:dirtyRect];
  } else {
    [self drawBackground:dirtyRect];
  }
}

- (void)drawAsDetachedBubble:(NSRect)dirtyRect {
  CGFloat morph = [controller_ detachedMorphProgress];
  Profile* profile = [controller_ profile];
  if (!profile)
    return;

  [[NSColor whiteColor] set];
  NSRectFill(dirtyRect);

  // Overlay with a lighter background color.
  const ui::ThemeProvider& tp =
      ThemeService::GetThemeProviderForProfile(profile);
  NSColor* toolbarColor =
      tp.GetNSColor(ThemeProperties::COLOR_DETACHED_BOOKMARK_BAR_BACKGROUND);
  CGFloat alpha = morph * [toolbarColor alphaComponent];
  [[toolbarColor colorWithAlphaComponent:alpha] set];
  NSRectFillUsingOperation(dirtyRect, NSCompositeSourceOver);

  // Fade in/out the background.
  {
    gfx::ScopedNSGraphicsContextSaveGState bgScopedState;
    NSGraphicsContext* context = [NSGraphicsContext currentContext];
    CGContextRef cgContext = static_cast<CGContextRef>([context graphicsPort]);
    CGContextSetAlpha(cgContext, 1 - morph);
    CGContextBeginTransparencyLayer(cgContext, NULL);
    [self drawBackground:dirtyRect];
    CGContextEndTransparencyLayer(cgContext);
  }

  // Bottom stroke.
  NSRect strokeRect = [self bounds];
  strokeRect.size.height = [self cr_lineWidth];
  if (NSIntersectsRect(strokeRect, dirtyRect)) {
    NSColor* strokeColor =
        tp.GetNSColor(ThemeProperties::COLOR_DETACHED_BOOKMARK_BAR_SEPARATOR);
    strokeColor = [[self strokeColor] blendedColorWithFraction:morph
                                                       ofColor:strokeColor];
    [strokeColor set];
    NSRectFillUsingOperation(NSIntersectionRect(strokeRect, dirtyRect),
                             NSCompositeSourceOver);
  }
}

@end  // @implementation BookmarkBarToolbarView
