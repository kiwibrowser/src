// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/omnibox_view_controller.h"

#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/omnibox/omnibox_container_view.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface OmniboxViewController ()

// Override of UIViewController's view with a different type.
@property(nonatomic, strong) OmniboxContainerView* view;

@property(nonatomic, strong) UIFont* textFieldFont;
@property(nonatomic, strong) UIColor* textFieldTintColor;
@property(nonatomic, strong) UIColor* textFieldTextColor;
@property(nonatomic, assign) BOOL incognito;

@end

@implementation OmniboxViewController
@synthesize textFieldFont = _textFieldFont;
@synthesize textFieldTintColor = _textFieldTintColor;
@synthesize textFieldTextColor = _textFieldTextColor;
@synthesize incognito = _incognito;
@dynamic view;

- (instancetype)initWithFont:(UIFont*)font
                   textColor:(UIColor*)textColor
                   tintColor:(UIColor*)tintColor
                   incognito:(BOOL)isIncognito {
  self = [super init];
  if (self) {
    _textFieldFont = font;
    _textFieldTextColor = textColor;
    _textFieldTintColor = tintColor;
    _incognito = isIncognito;
  }
  return self;
}

#pragma mark - UIViewController

- (void)loadView {
  self.view =
      [[OmniboxContainerView alloc] initWithFrame:CGRectZero
                                             font:self.textFieldFont
                                        textColor:self.textFieldTextColor
                                        tintColor:self.textFieldTintColor];
  self.view.incognito = self.incognito;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  [self.view setLeadingImageHidden:!IsIPadIdiom()];
  self.textField.placeholder = l10n_util::GetNSString(IDS_OMNIBOX_EMPTY_HINT);
}

#pragma mark - public methods

- (OmniboxTextFieldIOS*)textField {
  return self.view.textField;
}

#pragma mark - OmniboxConsumer

- (void)updateAutocompleteIcon:(UIImage*)icon {
  [self.view setLeadingImage:icon];
}

@end
