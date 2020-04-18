// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/app_launcher/open_mail_handler_view_controller.h"

#import <UIKit/UIKit.h>

#include "base/ios/block_types.h"
#include "base/logging.h"
#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_cell.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_handler_manager.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MDCPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Section IDs
typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierHeader = kSectionIdentifierEnumZero,
  SectionIdentifierApps,
  SectionIdentifierSwitch,
};

// Item IDs.
typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeTitle = kItemTypeEnumZero,
  ItemTypeApp,  // repeated item
  ItemTypeAlwaysAsk,
};

// Font size constants for text in the Bottom Sheet.
const CGFloat kBottomSheetTitleFontSize = 18.0f;
const CGFloat kAppNameFontSize = 16.0f;
const CGFloat kSwitchLabelFontSize = 12.0f;

}  // namespace

@interface OpenMailHandlerViewController () {
  // A manager object that can rewrite a mailto:// URL to one that can
  // launch a different mail client app.
  MailtoHandlerManager* _manager;
  // Item with the UISwitch toggle for user to choose whether a mailto://
  // app selection should be remembered for future use.
  CollectionViewSwitchItem* _alwaysAskItem;
  // An array of apps that can handle mailto:// URLs. The |_manager| object
  // lists all supported mailto:// handlers, but not all of them would be
  // installed.
  NSMutableArray<MailtoHandler*>* _availableHandlers;
  // Optional callback block that will be called after user has selected a
  // mailto:// handler.
  OpenMailtoHandlerSelectedHandler _selectedHandler;
}

// Callback function when the value of UISwitch |sender| in ItemTypeAlwaysAsk
// item is changed by the user.
- (void)didToggleAlwaysAskSwitch:(id)sender;
@end

@implementation OpenMailHandlerViewController

- (instancetype)initWithManager:(nullable MailtoHandlerManager*)manager
                selectedHandler:
                    (nullable OpenMailtoHandlerSelectedHandler)selectedHandler {
  self = [super initWithLayout:[[MDCCollectionViewFlowLayout alloc] init]
                         style:CollectionViewControllerStyleDefault];
  if (self) {
    _manager = manager;
    // This will be copied automatically by ARC.
    _selectedHandler = selectedHandler;
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];

  // TODO(crbug.com/765146): This is needed here because crrev/c/660257 is not
  // intended for M62 branch but this change to mailto:// handling is. This
  // change is redundant but can co-exist with crrev/c/660257.
  // This will be reverted after cherrypick to M62.
  if (@available(iOS 11, *)) {
    self.collectionView.contentInsetAdjustmentBehavior =
        UIScrollViewContentInsetAdjustmentNever;
  }

  [self loadModel];
}

- (void)loadModel {
  [super loadModel];
  CollectionViewModel* model = self.collectionViewModel;

  // Header
  [model addSectionWithIdentifier:SectionIdentifierHeader];
  CollectionViewTextItem* titleItem =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeTitle];
  titleItem.text = l10n_util::GetNSString(IDS_IOS_CHOOSE_EMAIL_APP);
  titleItem.textFont =
      [[MDCTypography fontLoader] mediumFontOfSize:kBottomSheetTitleFontSize];
  [model addItem:titleItem toSectionWithIdentifier:SectionIdentifierHeader];

  // Adds list of available mailto:// handler apps.
  [model addSectionWithIdentifier:SectionIdentifierApps];
  _availableHandlers = [NSMutableArray array];
  for (MailtoHandler* handler in [_manager defaultHandlers]) {
    if ([handler isAvailable]) {
      [_availableHandlers addObject:handler];
      CollectionViewTextItem* item =
          [[CollectionViewTextItem alloc] initWithType:ItemTypeApp];
      item.text = handler.appName;
      item.textFont =
          [[MDCTypography fontLoader] mediumFontOfSize:kAppNameFontSize];
      [model addItem:item toSectionWithIdentifier:SectionIdentifierApps];
    }
  }

  // The footer is a row with a toggle switch. The default should be always ask
  // for which app to choose, i.e. ON.
  [model addSectionWithIdentifier:SectionIdentifierSwitch];
  _alwaysAskItem =
      [[CollectionViewSwitchItem alloc] initWithType:ItemTypeAlwaysAsk];
  _alwaysAskItem.text = l10n_util::GetNSString(IDS_IOS_CHOOSE_EMAIL_ASK_TOGGLE);
  _alwaysAskItem.on = YES;
  [model addItem:_alwaysAskItem
      toSectionWithIdentifier:SectionIdentifierSwitch];
}

#pragma mark - UICollectionViewDataSource

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  UICollectionViewCell* cell =
      [super collectionView:collectionView cellForItemAtIndexPath:indexPath];

  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (itemType) {
    case ItemTypeTitle: {
      CollectionViewTextCell* textCell =
          base::mac::ObjCCastStrict<CollectionViewTextCell>(cell);
      // TODO(crbug.com/764325): Although text alignment for title is set to
      // centered, it is still appearing as left aligned.
      textCell.textLabel.textAlignment = NSTextAlignmentCenter;
      break;
    }
    case ItemTypeApp:
      break;
    case ItemTypeAlwaysAsk: {
      CollectionViewSwitchCell* switchCell =
          base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
      [switchCell.switchView addTarget:self
                                action:@selector(didToggleAlwaysAskSwitch:)
                      forControlEvents:UIControlEventValueChanged];
      switchCell.textLabel.font =
          [[MDCTypography fontLoader] mediumFontOfSize:kSwitchLabelFontSize];
      switchCell.textLabel.textColor = [[MDCPalette greyPalette] tint700];
      break;
    }
  }
  return cell;
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];

  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (itemType) {
    case ItemTypeApp: {
      NSUInteger row = indexPath.row;
      DCHECK_LT(row, [_availableHandlers count]);
      MailtoHandler* handler = _availableHandlers[row];
      if (![_alwaysAskItem isOn])
        [_manager setDefaultHandlerID:[handler appStoreID]];
      if (_selectedHandler)
        _selectedHandler(handler);
      [self.presentingViewController dismissViewControllerAnimated:YES
                                                        completion:nil];
      break;
    }
    case ItemTypeTitle:
    case ItemTypeAlwaysAsk:
      break;
  }
}

#pragma mark - MDCCollectionViewStylingDelegate

- (BOOL)collectionView:(UICollectionView*)collectionView
    hidesInkViewAtIndexPath:(NSIndexPath*)indexPath {
  // Disallow highlight (ripple effect) if the tapped row is not a mailto://
  // handler.
  return
      [self.collectionViewModel itemTypeForIndexPath:indexPath] != ItemTypeApp;
}

- (CGFloat)collectionView:(UICollectionView*)collectionView
    cellHeightAtIndexPath:(NSIndexPath*)indexPath {
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];
  UIEdgeInsets inset = [self collectionView:collectionView
                                     layout:collectionView.collectionViewLayout
                     insetForSectionAtIndex:indexPath.section];
  CGFloat width =
      CGRectGetWidth(collectionView.bounds) - inset.left - inset.right;
  return [MDCCollectionViewCell cr_preferredHeightForWidth:width forItem:item];
}

#pragma mark - Private

- (void)didToggleAlwaysAskSwitch:(UISwitch*)sender {
  BOOL isOn = [sender isOn];
  [_alwaysAskItem setOn:isOn];
}

@end
