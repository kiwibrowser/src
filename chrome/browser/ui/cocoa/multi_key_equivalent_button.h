// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_MULTI_KEY_EQUIVALENT_BUTTON_H_
#define CHROME_BROWSER_UI_COCOA_MULTI_KEY_EQUIVALENT_BUTTON_H_

#import <AppKit/AppKit.h>

#include <vector>

struct KeyEquivalentAndModifierMask {
 public:
  KeyEquivalentAndModifierMask() : charCode(nil), mask(0) {}
  NSString* charCode;
  NSUInteger mask;
};

// MultiKeyEquivalentButton is an NSButton subclass that is capable of
// responding to additional key equivalents.  It will respond to the ordinary
// NSButton key equivalent set by -setKeyEquivalent: and
// -setKeyEquivalentModifierMask:, and it will also respond to any additional
// equivalents provided to it in a KeyEquivalentAndModifierMask structure
// passed to -addKeyEquivalent:.

@interface MultiKeyEquivalentButton : NSButton {
 @private
  std::vector<KeyEquivalentAndModifierMask> extraKeys_;
}

- (void)addKeyEquivalent:(KeyEquivalentAndModifierMask)key;

@end

#endif  // CHROME_BROWSER_UI_COCOA_MULTI_KEY_EQUIVALENT_BUTTON_H_
