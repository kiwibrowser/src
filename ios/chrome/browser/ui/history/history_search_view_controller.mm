// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/history_search_view_controller.h"

#import "ios/chrome/browser/ui/history/history_search_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface HistorySearchViewController ()<UITextFieldDelegate> {
  // View displayed by the HistorySearchViewController
  HistorySearchView* _searchView;
}

// Action for the cancel button.
- (void)cancelButtonClicked:(id)sender;

@end

@implementation HistorySearchViewController
@synthesize delegate = _delegate;
@synthesize enabled = _enabled;

- (void)loadView {
  _searchView = [[HistorySearchView alloc] init];
  [_searchView setSearchBarDelegate:self];
  [_searchView setCancelTarget:self action:@selector(cancelButtonClicked:)];
  self.view = _searchView;
}

- (void)viewDidAppear:(BOOL)animated {
  [super viewDidAppear:animated];
  [_searchView becomeFirstResponder];
}

- (void)setEnabled:(BOOL)enabled {
  _enabled = enabled;
  [_searchView setEnabled:enabled];
}

- (void)cancelButtonClicked:(id)sender {
  [_searchView clearText];
  [_searchView endEditing:YES];
  [self.delegate historySearchViewControllerDidCancel:self];
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField*)textField
    shouldChangeCharactersInRange:(NSRange)range
                replacementString:(NSString*)string {
  NSMutableString* text = [NSMutableString stringWithString:[textField text]];
  [text replaceCharactersInRange:range withString:string];
  [self.delegate historySearchViewController:self didRequestSearchForTerm:text];
  return YES;
}

- (BOOL)textFieldShouldClear:(UITextField*)textField {
  [self.delegate historySearchViewController:self didRequestSearchForTerm:@""];
  return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField {
  [textField resignFirstResponder];
  return YES;
}

@end
