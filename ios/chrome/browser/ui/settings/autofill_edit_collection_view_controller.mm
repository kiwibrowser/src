// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/autofill_edit_collection_view_controller.h"

#include "base/logging.h"
#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/autofill/autofill_edit_accessory_view.h"
#import "ios/chrome/browser/ui/autofill/cells/autofill_edit_item.h"
#import "ios/chrome/browser/ui/settings/autofill_edit_collection_view_controller+protected.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

AutofillEditCell* AutofillEditCellForTextField(UITextField* textField) {
  AutofillEditCell* settingsCell = nil;
  for (UIView* view = textField; view; view = [view superview]) {
    AutofillEditCell* cell = base::mac::ObjCCast<AutofillEditCell>(view);
    if (cell) {
      settingsCell = cell;
      break;
    }
  }

  // There should be a cell associated with this text field.
  DCHECK(settingsCell);
  return settingsCell;
}

}  // namespace

@interface AutofillEditCollectionViewController ()<
    AutofillEditAccessoryDelegate> {
  AutofillEditCell* _currentEditingCell;
  AutofillEditAccessoryView* _accessoryView;
}
@end

@implementation AutofillEditCollectionViewController

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style {
  self = [super initWithLayout:layout style:style];
  if (!self) {
    return nil;
  }

  _accessoryView = [[AutofillEditAccessoryView alloc] initWithDelegate:self];
  [self setShouldHideDoneButton:YES];
  [self updateEditButton];
  return self;
}

- (void)viewDidAppear:(BOOL)animated {
  [super viewDidAppear:animated];
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(keyboardDidShow)
             name:UIKeyboardDidShowNotification
           object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
  [super viewWillDisappear:animated];
  [[NSNotificationCenter defaultCenter]
      removeObserver:self
                name:UIKeyboardDidShowNotification
              object:nil];
}

#pragma mark - SettingsRootCollectionViewController

- (BOOL)shouldShowEditButton {
  return YES;
}

- (BOOL)editButtonEnabled {
  return YES;
}

- (NSIndexPath*)indexForCellPathWithOffset:(NSInteger)offset
                                  fromPath:(NSIndexPath*)cellPath {
  if (!cellPath || !offset)
    return nil;

  NSInteger cellSection = [cellPath section];
  NSInteger nextCellRow = [cellPath row] + offset;

  if (nextCellRow >= 0 &&
      nextCellRow < [[self collectionView] numberOfItemsInSection:cellSection])
    return [NSIndexPath indexPathForRow:nextCellRow inSection:cellSection];

  NSInteger nextCellSection = cellSection + offset;
  if (nextCellSection >= 0 &&
      nextCellSection < [[self collectionView] numberOfSections])
    return [NSIndexPath indexPathForRow:0 inSection:nextCellSection];

  return nil;
}

#pragma mark - UITextFieldDelegate

- (void)textFieldDidBeginEditing:(UITextField*)textField {
  AutofillEditCell* cell = AutofillEditCellForTextField(textField);
  _currentEditingCell = cell;
  [textField setInputAccessoryView:_accessoryView];
  [self updateAccessoryViewButtonState];
}

- (void)textFieldDidEndEditing:(UITextField*)textField {
  AutofillEditCell* cell = AutofillEditCellForTextField(textField);
  DCHECK(_currentEditingCell == cell);
  [textField setInputAccessoryView:nil];
  _currentEditingCell = nil;
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField {
  DCHECK([_currentEditingCell textField] == textField);
  [self nextPressed];
  return NO;
}

#pragma mark - AutofillEditAccessoryDelegate

- (void)nextPressed {
  [self moveToAnotherCellWithOffset:1];
}

- (void)previousPressed {
  [self moveToAnotherCellWithOffset:-1];
}

- (void)closePressed {
  [[_currentEditingCell textField] resignFirstResponder];
}

#pragma mark - Helper methods

- (NSIndexPath*)indexPathForCurrentTextField {
  DCHECK(_currentEditingCell);
  return [[self collectionView] indexPathForCell:_currentEditingCell];
}

- (void)moveToAnotherCellWithOffset:(NSInteger)offset {
  UICollectionView* collectionView = [self collectionView];
  NSIndexPath* cellPath = [self indexPathForCurrentTextField];
  DCHECK(cellPath);
  NSIndexPath* nextCellPath =
      [self indexForCellPathWithOffset:offset fromPath:cellPath];

  if (!nextCellPath) {
    [[_currentEditingCell textField] resignFirstResponder];
  } else {
    AutofillEditCell* nextCell = base::mac::ObjCCastStrict<AutofillEditCell>(
        [collectionView cellForItemAtIndexPath:nextCellPath]);
    [nextCell.textField becomeFirstResponder];
  }
}

- (void)updateAccessoryViewButtonState {
  NSIndexPath* currentPath = [self indexPathForCurrentTextField];
  NSIndexPath* nextPath =
      [self indexForCellPathWithOffset:1 fromPath:currentPath];
  NSIndexPath* previousPath =
      [self indexForCellPathWithOffset:-1 fromPath:currentPath];

  [[_accessoryView previousButton] setEnabled:previousPath != nil];
  [[_accessoryView nextButton] setEnabled:nextPath != nil];
}

#pragma mark - Keyboard handling

- (void)keyboardDidShow {
  [self.collectionView scrollRectToVisible:[_currentEditingCell frame]
                                  animated:YES];
}

@end
