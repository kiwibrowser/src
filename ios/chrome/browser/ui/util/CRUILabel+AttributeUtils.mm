// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/util/CRUILabel+AttributeUtils.h"

#import <objc/runtime.h>

#include "base/logging.h"
#import "ios/chrome/browser/ui/util/label_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// They key under which to associate the line height with the label.
const void* const kLineHeightKey = &kLineHeightKey;
// Creates an NSNumber from |line_height| and associates it with |label|.
void SetAssociatedLineHeight(CGFloat line_height, UILabel* label) {
  objc_setAssociatedObject(label, kLineHeightKey, @(line_height),
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
// Returns the line height associated with |label|.
CGFloat GetAssociatedLineHeight(UILabel* label) {
  return [objc_getAssociatedObject(label, kLineHeightKey) floatValue];
}
}

@implementation UILabel (CRUILabelAttributeUtils)

// If there's no text in the label, the line height is whatever is stored in the
// associated value.
- (CGFloat)cr_lineHeight {
  if (!self.text.length || !self.attributedText.string.length)
    return GetAssociatedLineHeight(self);
  NSParagraphStyle* style =
      [self.attributedText attribute:NSParagraphStyleAttributeName
                             atIndex:0
                      effectiveRange:nullptr];
  return style.maximumLineHeight;
}

- (void)cr_setLineHeight:(CGFloat)lineHeight {
  DCHECK_GT(lineHeight, 0.0);
  // If this is the first time the line height is being set, register the
  // LabelObserverAction.
  if (!GetAssociatedLineHeight(self)) {
    LabelObserver* observer = [LabelObserver observerForLabel:self];
    [observer addTextChangedAction:^(UILabel* label) {
      label.cr_lineHeight = GetAssociatedLineHeight(label);
    }];
  }

  // Store the new line height as an associated object.
  SetAssociatedLineHeight(lineHeight, self);

  // If there's no text yet, there's nothing that can be done.  The
  // LabelObserverAction will call this selector again upon changes to the text.
  if (!self.text.length || !self.attributedText.string.length)
    return;

  NSMutableAttributedString* newString = [self.attributedText mutableCopy];
  DCHECK([newString length]);
  NSParagraphStyle* style = [newString attribute:NSParagraphStyleAttributeName
                                         atIndex:0
                                  effectiveRange:nullptr];
  if (!style)
    style = [NSParagraphStyle defaultParagraphStyle];
  NSMutableParagraphStyle* newStyle = [style mutableCopy];
  [newStyle setMinimumLineHeight:lineHeight];
  [newStyle setMaximumLineHeight:lineHeight];
  [newString addAttribute:NSParagraphStyleAttributeName
                    value:newStyle
                    range:NSMakeRange(0, [newString length])];
  self.attributedText = newString;
}

@end
