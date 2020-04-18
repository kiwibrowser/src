// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/toolbar/app_toolbar_button_cell.h"

#include "base/macros.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "ui/gfx/canvas_skia_paint.h"
#include "ui/gfx/geometry/rect.h"

@interface AppToolbarButtonCell ()
- (void)commonInit;
@end

@implementation AppToolbarButtonCell

- (id)initTextCell:(NSString*)text {
  if ((self = [super initTextCell:text])) {
    [self commonInit];
  }
  return self;
}

- (id)initWithCoder:(NSCoder*)decoder {
  if ((self = [super initWithCoder:decoder])) {
    [self commonInit];
  }
  return self;
}

- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView*)controlView {
  gfx::CanvasSkiaPaint canvas(cellFrame, false);
  canvas.set_composite_alpha(true);
  canvas.SaveLayerAlpha(255 *
                        [self imageAlphaForWindowState:[controlView window]]);

  canvas.Restore();
}

- (void)commonInit {
}

@end
