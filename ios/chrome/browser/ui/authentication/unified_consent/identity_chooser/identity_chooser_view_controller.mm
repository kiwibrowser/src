// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_view_controller.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_view_controller_presentation_delegate.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_view_controller_selection_delegate.h"
#import "ios/third_party/material_components_ios/src/components/Dialogs/src/MaterialDialogs.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kViewControllerWidth = 312.;
const CGFloat kViewControllerHeight = 230.;
// Header height for identity section.
const CGFloat kHeaderHeight = 49.;
// Footer height for "Add Account…" section.
const CGFloat kFooterHeight = 17.;
}  // namespace

@interface IdentityChooserViewController ()

@property(nonatomic, strong)
    MDCDialogTransitionController* transitionController;

@end

@implementation IdentityChooserViewController

@synthesize presentationDelegate = _presentationDelegate;
@synthesize selectionDelegate = _selectionDelegate;
@synthesize transitionController = _transitionController;

- (instancetype)init {
  self = [super initWithTableViewStyle:UITableViewStylePlain
                           appBarStyle:ChromeTableViewControllerStyleNoAppBar];
  if (self) {
    self.modalPresentationStyle = UIModalPresentationCustom;
    _transitionController = [[MDCDialogTransitionController alloc] init];
    self.transitioningDelegate = _transitionController;
    self.modalPresentationStyle = UIModalPresentationCustom;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.preferredContentSize =
      CGSizeMake(kViewControllerWidth, kViewControllerHeight);
  self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
  self.tableView.contentInset = UIEdgeInsetsMake(0, 0, kFooterHeight, 0);
  self.tableView.sectionFooterHeight = 0;
}

- (void)viewDidDisappear:(BOOL)animated {
  [self.presentationDelegate identityChooserViewControllerDidDisappear:self];
}

- (CGFloat)tableView:(UITableView*)tableView
    heightForHeaderInSection:(NSInteger)section {
  return (section == 0) ? kHeaderHeight : 0;
}

- (void)tableView:(UITableView*)tableView
    didSelectRowAtIndexPath:(NSIndexPath*)indexPath {
  // TODO(crbug.com/827072): Needs to implement "Add Account…" button action.
  [self.tableView deselectRowAtIndexPath:indexPath animated:YES];
  if (indexPath.section != 0)
    return;
  [self.selectionDelegate identityChooserViewController:self
                           didSelectIdentityAtIndexPath:indexPath];
}

@end
