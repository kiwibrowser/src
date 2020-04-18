// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/clipping_textfield.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ClippingTextField
@synthesize clippingTextfieldDelegate = _clippingTextfieldDelegate;

- (void)setText:(NSString*)setText {
  [super setText:setText];
  [self.clippingTextfieldDelegate textFieldTextChanged:self];
}

- (void)setAttributedText:(NSAttributedString*)attributedText {
  [super setAttributedText:attributedText];
  [self.clippingTextfieldDelegate textFieldTextChanged:self];
}

- (BOOL)becomeFirstResponder {
  BOOL didBecomeFirstResponder = [super becomeFirstResponder];
  [self.clippingTextfieldDelegate textFieldBecameFirstResponder:self];
  return didBecomeFirstResponder;
}

- (BOOL)resignFirstResponder {
  BOOL didResignFirstResponder = [super resignFirstResponder];
  if (didResignFirstResponder) {
    [self.clippingTextfieldDelegate textFieldResignedFirstResponder:self];
  }
  return didResignFirstResponder;
}

@end
