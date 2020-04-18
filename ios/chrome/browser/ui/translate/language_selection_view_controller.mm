// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/translate/language_selection_view_controller.h"

#import "base/logging.h"
#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/translate/language_selection_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kLanguagePickerCancelButtonId = @"LanguagePickerCancelButton";
NSString* const kLanguagePickerDoneButtonId = @"LanguagePickerDoneButton";

namespace {
CGFloat kUIPickerFontSize = 26;
}

@interface LanguageSelectionViewController ()<UIPickerViewDataSource,
                                              UIPickerViewDelegate> {
  // YES if NSLayoutConstraits were added.
  BOOL _addedConstraints;
}

@property(nonatomic, weak) UIPickerView* picker;

// Action methods for navigation bar buttons.
- (void)languageSelectionDone;
- (void)languageSelectionCancelled;

@end

@implementation LanguageSelectionViewController

// Synthesize properties defined by LanguageSelectionConsumer
@synthesize languageCount = _languageCount;
@synthesize initialLanguageIndex = _initialLanguageIndex;
@synthesize disabledLanguageIndex = _disabledLanguageIndex;
@synthesize provider = _provider;
// Synthesize public properties
@synthesize delegate = _delegate;
// Synthesize private properties
@synthesize picker = _picker;

#pragma mark - UIViewController

- (void)viewDidLoad {
  DCHECK(_languageCount && _provider);

  UIPickerView* picker = [[UIPickerView alloc] initWithFrame:CGRectZero];
  picker.backgroundColor = UIColor.whiteColor;
  picker.translatesAutoresizingMaskIntoConstraints = NO;
  picker.showsSelectionIndicator = YES;
  picker.dataSource = self;
  picker.delegate = self;
  [picker selectRow:self.initialLanguageIndex inComponent:0 animated:NO];

  [self.view addSubview:picker];
  self.picker = picker;

  UINavigationBar* bar = [[UINavigationBar alloc] initWithFrame:CGRectZero];
  bar.translatesAutoresizingMaskIntoConstraints = NO;

  UIBarButtonItem* doneButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemDone
                           target:self
                           action:@selector(languageSelectionDone)];
  [doneButton setAccessibilityIdentifier:kLanguagePickerDoneButtonId];

  UIBarButtonItem* cancelButton = [[UIBarButtonItem alloc]
      initWithBarButtonSystemItem:UIBarButtonSystemItemCancel
                           target:self
                           action:@selector(languageSelectionCancelled)];
  [cancelButton setAccessibilityIdentifier:kLanguagePickerCancelButtonId];

  UINavigationItem* item = [[UINavigationItem alloc] initWithTitle:@""];
  [item setRightBarButtonItem:doneButton];
  [item setLeftBarButtonItem:cancelButton];
  [item setHidesBackButton:YES];
  [bar pushNavigationItem:item animated:NO];

  [self.view addSubview:bar];

  // Bar sits on top of the picker, both are full width. Constraints don't
  // change. Height is entirely determined by the preferred content sizes of
  // the bar and picker.
  [NSLayoutConstraint activateConstraints:@[
    [bar.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
    [picker.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
    [bar.widthAnchor constraintEqualToAnchor:self.view.widthAnchor],
    [picker.widthAnchor constraintEqualToAnchor:self.view.widthAnchor],
    [bar.topAnchor constraintEqualToAnchor:self.view.topAnchor],
    [picker.topAnchor constraintEqualToAnchor:bar.bottomAnchor],
    [picker.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
  ]];
}

- (void)updateViewConstraints {
  if (!_addedConstraints) {
    [[self.view.superview.widthAnchor
        constraintEqualToAnchor:self.view.widthAnchor] setActive:YES];
    _addedConstraints = YES;
  }
  [super updateViewConstraints];
}

#pragma mark - UIPickerViewDataSource

- (NSInteger)pickerView:(UIPickerView*)pickerView
    numberOfRowsInComponent:(NSInteger)component {
  return _languageCount;
}

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView*)pickerView {
  return 1;
}

#pragma mark - UIPickerViewDelegate

- (UIView*)pickerView:(UIPickerView*)pickerView
           viewForRow:(NSInteger)row
         forComponent:(NSInteger)component
          reusingView:(UIView*)view {
  DCHECK_EQ(0, component);
  UILabel* label = base::mac::ObjCCast<UILabel>(view) ?: [[UILabel alloc] init];
  label.text = [_provider languageNameAtIndex:row];
  label.textAlignment = NSTextAlignmentCenter;
  UIFont* font = [UIFont systemFontOfSize:kUIPickerFontSize];
  if (row == _initialLanguageIndex) {
    font = [UIFont boldSystemFontOfSize:kUIPickerFontSize];
  } else if (row == _disabledLanguageIndex) {
    label.enabled = NO;
  }
  label.font = font;
  return label;
}

#pragma mark - Navigation buttons

- (void)languageSelectionDone {
  [self.delegate
      languageSelectedAtIndex:[self.picker selectedRowInComponent:0]];
}

- (void)languageSelectionCancelled {
  [self.delegate languageSelectionCanceled];
}

@end
