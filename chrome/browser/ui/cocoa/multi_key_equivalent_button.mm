// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#import "chrome/browser/ui/cocoa/multi_key_equivalent_button.h"

@implementation MultiKeyEquivalentButton

- (void)addKeyEquivalent:(KeyEquivalentAndModifierMask)key {
  extraKeys_.push_back(key);
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
  NSWindow* modalWindow = [NSApp modalWindow];
  NSWindow* window = [self window];

  if ([self isEnabled] &&
      (!modalWindow || modalWindow == window || [window worksWhenModal])) {
    for (size_t index = 0; index < extraKeys_.size(); ++index) {
      KeyEquivalentAndModifierMask key = extraKeys_[index];
      if (key.charCode &&
          [key.charCode isEqualToString:[event charactersIgnoringModifiers]] &&
          ([event modifierFlags] & key.mask) == key.mask) {
        [self performClick:self];
        return YES;
      }
    }
  }

  return [super performKeyEquivalent:event];
}

@end
