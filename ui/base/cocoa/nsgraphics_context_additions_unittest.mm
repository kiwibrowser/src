// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/nsgraphics_context_additions.h"

#include "base/mac/scoped_nsobject.h"
#import "ui/base/test/cocoa_helper.h"

typedef ui::CocoaTest NSGraphicsContextCrAdditionsTest;

TEST_F(NSGraphicsContextCrAdditionsTest, PatternPhase) {
  NSRect frame = NSMakeRect(50, 50, 100, 100);
  base::scoped_nsobject<NSView> view([[NSView alloc] initWithFrame:frame]);
  [[test_window() contentView] addSubview:view];

  [view lockFocus];
  NSGraphicsContext* context = [NSGraphicsContext currentContext];

  [context cr_setPatternPhase:NSZeroPoint forView:view];
  EXPECT_EQ(0, [context patternPhase].y);

  [view setWantsLayer:YES];
  [context cr_setPatternPhase:NSZeroPoint forView:view];
  EXPECT_EQ(-NSMinY(frame), [context patternPhase].y);

  [view unlockFocus];
}
