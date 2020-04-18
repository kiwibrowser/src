// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/nsgraphics_context_additions.h"

#include "base/logging.h"

@implementation NSGraphicsContext (CrAdditions)

- (void)cr_setPatternPhase:(NSPoint)phase
                   forView:(NSView*)view {
  // TODO(sdy): Remove once we no longer add the tab background view to
  // [[window contentView] superview] *or* no longer make the content view
  // smaller than the window while exiting fullscreen. These two things
  // together can result in the tab background view being drawn while it's
  // outside of contentView, with a pattern phase that assumes it's inside.
  NSView* contentView = [[view window] contentView];
  if (![view isDescendantOf:contentView]) {
    NSView* frameView = [contentView superview];
    DCHECK([view isDescendantOf:frameView]);
    // Convert phase into an offset from the top left corner of contentView so
    // that it will be aligned correctly at the end of the transition.
    phase.x += NSMinX([frameView frame]) - NSMinX([contentView frame]);
    phase.y += NSMaxY([frameView frame]) - NSMaxY([contentView frame]);
  }

  NSView* ancestorWithLayer = view;
  while (ancestorWithLayer && ![ancestorWithLayer layer])
    ancestorWithLayer = [ancestorWithLayer superview];
  if (ancestorWithLayer) {
    NSPoint bottomLeft = NSZeroPoint;
    if ([ancestorWithLayer isFlipped])
      bottomLeft.y = NSMaxY([ancestorWithLayer bounds]);
    NSPoint offset = [ancestorWithLayer convertPoint:bottomLeft toView:nil];
    phase.x -= offset.x;
    phase.y -= offset.y;
  }
  [self setPatternPhase:phase];
}

@end
