// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/clipping_textfield_container.h"

#include "base/strings/sys_string_conversions.h"
#include "components/omnibox/browser/autocomplete_input.h"
#include "ios/chrome/browser/autocomplete/autocomplete_scheme_classifier_impl.h"
#import "ios/chrome/browser/ui/omnibox/clipping_mask_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ClippingTextFieldContainer ()

@property(nonatomic, strong) NSLayoutConstraint* leftConstraint;
@property(nonatomic, strong) NSLayoutConstraint* rightConstraint;
@property(nonatomic, strong) ClippingMaskView* gradientMaskView;
@property(nonatomic, assign, getter=isClipping) BOOL clipping;

@end

@implementation ClippingTextFieldContainer
@synthesize textField = _textField;
@synthesize leftConstraint = _leftConstraint;
@synthesize rightConstraint = _rightConstraint;
@synthesize gradientMaskView = _gradientMaskView;
@synthesize clipping = _clipping;

- (instancetype)initWithClippingTextField:(ClippingTextField*)textField {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    _textField = textField;
    textField.clippingTextfieldDelegate = self;
    [self addSubview:textField];

    _gradientMaskView = [[ClippingMaskView alloc] init];
    self.maskView = _gradientMaskView;

    // Configure layout.
    // Left and Right are used instead of Leading and Trailing because the
    // clipping only ever applies to URLs that are always displayed in LTR.
    _leftConstraint =
        [self.textField.leftAnchor constraintEqualToAnchor:self.leftAnchor];
    _rightConstraint =
        [self.textField.rightAnchor constraintEqualToAnchor:self.rightAnchor];
    [NSLayoutConstraint activateConstraints:@[
      [textField.topAnchor constraintEqualToAnchor:self.topAnchor],
      [textField.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
      _leftConstraint,
      _rightConstraint,
    ]];
    textField.translatesAutoresizingMaskIntoConstraints = NO;
    self.clipsToBounds = YES;
    _clipping = NO;
  }
  return self;
}

- (void)layoutSubviews {
  if ([self isClipping]) {
    [self applyClipping];
  }
  [super layoutSubviews];
  self.gradientMaskView.frame = self.bounds;
}

- (void)startClipping {
  self.clipping = YES;
  [self applyClipping];
  [self setNeedsLayout];
  [self layoutIfNeeded];
}

- (void)applyClipping {
  CGFloat suffixWidth = 0;
  CGFloat prefixWidth =
      -[self leftConstantWithAttributedText:self.textField.attributedText
                              rightConstant:&suffixWidth];

  [self applyGradientsToPrefix:(prefixWidth < 0) suffix:(suffixWidth > 0)];

  self.leftConstraint.constant = prefixWidth;
  self.rightConstraint.constant = suffixWidth;
}

- (void)stopClipping {
  if (![self isClipping]) {
    return;
  }
  self.clipping = NO;
  self.leftConstraint.constant = 0;
  self.rightConstraint.constant = 0;
  [self removeGradient];
}

// Fade the beginning and/or end of the visible string to indicate to the user
// that the URL has been clipped.
- (void)applyGradientsToPrefix:(BOOL)shouldApplyToPrefix
                        suffix:(BOOL)shouldApplyToSuffix {
  self.gradientMaskView.fadeLeft = shouldApplyToPrefix;
  self.gradientMaskView.fadeRight = shouldApplyToSuffix;
}

- (void)removeGradient {
  [self applyGradientsToPrefix:NO suffix:NO];
}

#pragma mark calculate clipping

// Calculates the length (in pts) of the clipped text on the left and
// right sides of the omnibox in order to show the most significant part of
// the hostname.
- (CGFloat)leftConstantWithAttributedText:(NSAttributedString*)attributedText
                            rightConstant:(out CGFloat*)right {
  // The goal is to always show the most significant part of the hostname
  // (i.e. the end of the TLD).
  //
  //                     --------------------
  // www.somereallyreally|longdomainname.com|/path/gets/clipped
  //                     --------------------
  // {  clipped prefix  } {  visible text  } { clipped suffix }

  // First find how much (if any) of the scheme/host needs to be clipped so that
  // the end of the TLD fits in bounds.

  CGFloat widthOfClippedPrefix = 0;
  url::Component scheme, host;
  AutocompleteInput::ParseForEmphasizeComponents(
      base::SysNSStringToUTF16(attributedText.string),
      AutocompleteSchemeClassifierImpl(), &scheme, &host);
  if (host.len < 0) {
    return 0;
  }
  NSRange hostRange = NSMakeRange(0, host.begin + host.len);
  NSAttributedString* hostString =
      [attributedText attributedSubstringFromRange:hostRange];
  CGFloat widthOfHost = ceil(hostString.size.width);
  widthOfClippedPrefix = MAX(widthOfHost - self.bounds.size.width, 0);

  // Now determine if there is any text that will need to be truncated because
  // there's not enough room.
  int textWidth = ceil(attributedText.size.width);
  CGFloat widthOfClippedSuffix =
      MAX(textWidth - self.bounds.size.width - widthOfClippedPrefix, 0);
  *right = widthOfClippedSuffix;
  return widthOfClippedPrefix;
}

#pragma mark - ClippingTextFieldDelegate

- (void)textFieldTextChanged:(UITextField*)sender {
  [self startClipping];
}

- (void)textFieldBecameFirstResponder:(UITextField*)sender {
  [self stopClipping];
}

- (void)textFieldResignedFirstResponder:(UITextField*)sender {
  [self startClipping];
}

@end
