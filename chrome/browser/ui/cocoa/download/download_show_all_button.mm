// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/download_show_all_button.h"

#include "base/logging.h"
#import "chrome/browser/ui/cocoa/download/download_show_all_cell.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/grit/theme_resources.h"
#import "ui/base/cocoa/nsview_additions.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"

@implementation DownloadShowAllButton

- (void)awakeFromNib {
  DCHECK([[self cell] isKindOfClass:[DownloadShowAllCell class]]);
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  NSImage* favicon = rb.GetNativeImageNamed(IDR_DOWNLOADS_FAVICON).ToNSImage();
  [self setImage:favicon];
}

// GTM's layout tweaker calls sizeToFit to receive the desired width of views.
// By default, buttons will be only 14px high, but the Show All button needs to
// be higher.
- (void)sizeToFit {
  NSRect oldRect = [self frame];
  [super sizeToFit];
  NSRect newRect = [self frame];

  // Keep old height.
  newRect.origin.y = oldRect.origin.y;
  newRect.size.height = oldRect.size.height;

  [self setFrame:newRect];
}

- (BOOL)isOpaque {
  // Make this control opaque so that sub-pixel anti-aliasing works when
  // CoreAnimation is enabled.
  return YES;
}

- (void)drawRect:(NSRect)rect {
  NSView* downloadShelfView = [self ancestorWithViewID:VIEW_ID_DOWNLOAD_SHELF];
  // Previously the show all button used cr_drawUsingAncestor:inRect: to use
  // the download shelf to draw its background. However when the download
  // shelf has zero height, the shelf's drawing methods don't work as expected
  // because they constrain their drawing to the shelf's bounds rect. This
  // situation occurs sometimes when the shelf is about to become visible,
  // and the result is a very dark show all button
  //
  // To work around this problem, we'll call a variant of that method which
  // does restrict drawing to the ancestor view's bounds.
  [self cr_drawUsingAncestor:downloadShelfView
                      inRect:rect
     clippedToAncestorBounds:NO];
  [super drawRect:rect];
}

// ThemedWindowDrawing implementation.

- (void)windowDidChangeTheme {
  [self setNeedsDisplay:YES];
}

- (void)windowDidChangeActive {
  [self setNeedsDisplay:YES];
}

@end
