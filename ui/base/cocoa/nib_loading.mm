// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/nib_loading.h"

#include "base/mac/bundle_locations.h"
#include "base/mac/scoped_nsobject.h"
#include "base/mac/sdk_forward_declarations.h"

namespace ui {

NSView* GetViewFromNib(NSString* name) {
  base::scoped_nsobject<NSNib> nib(
      [[NSNib alloc] initWithNibNamed:name
                               bundle:base::mac::FrameworkBundle()]);
  if (!nib)
    return nil;

  NSArray* objects;
  BOOL success = [nib instantiateWithOwner:nil topLevelObjects:&objects];
  if (!success)
    return nil;

  // For some strange reason, even nibs that appear to have but one top-level
  // object often have more (an NSApplication, etc.). Filter out what isn't
  // desired.
  for (NSView* view in objects) {
    if (![view isKindOfClass:[NSView class]])
      continue;

    return [[view retain] autorelease];
  }

  return nil;
}

}  // namespace ui
