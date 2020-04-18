// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/autofill/form_suggestion_view.h"

#include "base/i18n/rtl.h"
#import "components/autofill/ios/browser/form_suggestion.h"
#import "ios/chrome/browser/autofill/form_suggestion_label.h"
#import "ios/chrome/browser/autofill/form_suggestion_view_client.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Vertical margin between suggestions and the edge of the suggestion content
// frame.
const CGFloat kSuggestionVerticalMargin = 6;

// Horizontal margin around suggestions (i.e. between suggestions, and between
// the end suggestions and the suggestion content frame).
const CGFloat kSuggestionHorizontalMargin = 6;

}  // namespace

@implementation FormSuggestionView {
  // The FormSuggestions that are displayed by this view.
  NSArray* _suggestions;
}

- (instancetype)initWithFrame:(CGRect)frame
                       client:(id<FormSuggestionViewClient>)client
                  suggestions:(NSArray*)suggestions {
  self = [super initWithFrame:frame];
  if (self) {
    _suggestions = [suggestions copy];

    self.showsVerticalScrollIndicator = NO;
    self.showsHorizontalScrollIndicator = NO;
    self.bounces = NO;
    self.canCancelContentTouches = YES;

    // Total height occupied by the label content, padding, border and margin.
    const CGFloat labelHeight =
        CGRectGetHeight(frame) - kSuggestionVerticalMargin * 2;

    __block CGFloat currentX = kSuggestionHorizontalMargin;
    void (^setupBlock)(FormSuggestion* suggestion, NSUInteger idx, BOOL* stop) =
        ^(FormSuggestion* suggestion, NSUInteger idx, BOOL* stop) {
          // FormSuggestionLabel will adjust the width, so here 0 is used for
          // the width.
          CGRect proposedFrame =
              CGRectMake(currentX, kSuggestionVerticalMargin, 0, labelHeight);
          UIView* label = [[FormSuggestionLabel alloc]
              initWithSuggestion:suggestion
                   proposedFrame:proposedFrame
                           index:idx
                  numSuggestions:[_suggestions count]
                          client:client];
          [self addSubview:label];
          currentX +=
              CGRectGetWidth([label frame]) + kSuggestionHorizontalMargin;
        };
    [_suggestions
        enumerateObjectsWithOptions:(base::i18n::IsRTL() ? NSEnumerationReverse
                                                         : 0)
                         usingBlock:setupBlock];
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];

  CGRect frame = self.frame;

  CGFloat contentwidth = kSuggestionHorizontalMargin;
  for (UIView* label in self.subviews) {
    contentwidth += CGRectGetWidth([label frame]) + kSuggestionHorizontalMargin;
  }

  if (base::i18n::IsRTL()) {
    if (contentwidth < CGRectGetWidth(frame)) {
      self.contentSize = frame.size;
      // Offsets labels for right alignment.
      CGFloat offset = CGRectGetWidth(frame) - contentwidth;
      CGFloat currentX = kSuggestionHorizontalMargin + offset;
      for (UIView* label in self.subviews) {
        CGRect newFrame = label.frame;
        newFrame.origin.x = currentX;
        label.frame = newFrame;
        currentX += CGRectGetWidth(label.frame) + kSuggestionHorizontalMargin;
      }
    } else {
      self.contentSize = CGSizeMake(contentwidth, CGRectGetHeight(frame));
      // Sets the visible rectangle so suggestions at the right end are
      // initially visible.
      CGRect initRect = {{contentwidth - CGRectGetWidth(frame), 0}, frame.size};
      [self scrollRectToVisible:initRect animated:NO];
    }
  } else {
    self.contentSize = CGSizeMake(contentwidth, CGRectGetHeight(frame));
  }
}

- (NSArray*)suggestions {
  return _suggestions;
}

@end
