// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/authentication/signin_account_selector_view_controller.h"

#include <memory>

#import "base/mac/foundation_util.h"
#import "ios/chrome/browser/signin/chrome_identity_service_observer_bridge.h"
#import "ios/chrome/browser/ui/authentication/resized_avatar_cache.h"
#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_account_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#import "ios/public/provider/chrome/browser/signin/chrome_identity.h"
#include "ios/public/provider/chrome/browser/signin/chrome_identity_service.h"
#import "ios/third_party/material_components_ios/src/components/AppBar/src/MaterialAppBar.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#import "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kHeaderViewMinHeight = 100.;
const CGFloat kHeaderViewHeightMultiplier = 0.33;
const CGFloat kContentViewBottomInset = 40.;

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierTitle = kSectionIdentifierEnumZero,
  SectionIdentifierAccounts,
  SectionIdentifierAddAccount,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeTitle = kItemTypeEnumZero,
  ItemTypeAccount,
  ItemTypeAddAccount,
};
}  // namespace

@interface SigninAccountSelectorViewController ()<
    ChromeIdentityServiceObserver> {
  std::unique_ptr<ChromeIdentityServiceObserverBridge> _identityServiceObserver;
  // Cache for account avatar images.
  ResizedAvatarCache* _avatarCache;
}
@end

@implementation SigninAccountSelectorViewController

@synthesize delegate = _delegate;

- (instancetype)init {
  UICollectionViewLayout* layout = [[MDCCollectionViewFlowLayout alloc] init];
  self =
      [super initWithLayout:layout style:CollectionViewControllerStyleAppBar];
  if (self) {
    _identityServiceObserver.reset(
        new ChromeIdentityServiceObserverBridge(self));
    _avatarCache = [[ResizedAvatarCache alloc] init];
  }
  return self;
}

#pragma mark - UIViewController

- (void)viewDidLoad {
  [super viewDidLoad];

  // Configure the header.
  MDCFlexibleHeaderView* headerView =
      self.appBar.headerViewController.headerView;
  headerView.canOverExtend = YES;
  headerView.maximumHeight = 200;
  headerView.backgroundColor = [UIColor whiteColor];
  headerView.shiftBehavior = MDCFlexibleHeaderShiftBehaviorEnabled;
  [headerView addSubview:[self contentViewWithFrame:headerView.bounds]];
  self.appBar.navigationBar.hidesBackButton = YES;
  self.collectionView.backgroundColor = [UIColor clearColor];
  [headerView changeContentInsets:^{
    UIEdgeInsets contentInset = self.collectionView.contentInset;
    contentInset.bottom += kContentViewBottomInset;
    self.collectionView.contentInset = contentInset;
  }];

  // Load the contents of the collection view.
  [self loadModel];
}

- (UIView*)contentViewWithFrame:(CGRect)frame {
  UIView* contentView = [[UIView alloc] initWithFrame:frame];
  contentView.autoresizingMask =
      (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
  contentView.clipsToBounds = YES;

  UILabel* titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
  titleLabel.text =
      l10n_util::GetNSString(IDS_IOS_ACCOUNT_CONSISTENCY_SETUP_TITLE);
  titleLabel.textColor = [[MDCPalette greyPalette] tint900];
  titleLabel.font = [MDCTypography headlineFont];
  titleLabel.translatesAutoresizingMaskIntoConstraints = NO;

  UIView* divider = [[UIView alloc] initWithFrame:CGRectZero];
  divider.backgroundColor = [[MDCPalette greyPalette] tint300];
  divider.translatesAutoresizingMaskIntoConstraints = NO;

  UILayoutGuide* layoutGuide1 = [[UILayoutGuide alloc] init];
  UILayoutGuide* layoutGuide2 = [[UILayoutGuide alloc] init];

  [contentView addSubview:titleLabel];
  [contentView addSubview:divider];
  [contentView addLayoutGuide:layoutGuide1];
  [contentView addLayoutGuide:layoutGuide2];

  NSDictionary* views = @{
    @"title" : titleLabel,
    @"divider" : divider,
    @"v1" : layoutGuide1,
    @"v2" : layoutGuide2
  };
  NSArray* constraints = @[
    @"V:[title]-(16)-[divider(==1)]|",
    @"H:|[v1(16)][title(<=440)][v2(>=v1)]|",
    @"H:|[divider]|",
  ];
  ApplyVisualConstraints(constraints, views);
  return contentView;
}

- (void)viewWillLayoutSubviews {
  CGSize viewSize = self.view.bounds.size;
  MDCFlexibleHeaderView* headerView =
      self.appBar.headerViewController.headerView;
  headerView.maximumHeight =
      MAX(kHeaderViewMinHeight, kHeaderViewHeightMultiplier * viewSize.height);
}

#pragma mark - Model

- (void)loadModel {
  [super loadModel];
  CollectionViewModel* model = self.collectionViewModel;
  [model addSectionWithIdentifier:SectionIdentifierTitle];
  [model addItem:[self titleItem]
      toSectionWithIdentifier:SectionIdentifierTitle];

  [model addSectionWithIdentifier:SectionIdentifierAccounts];
  NSArray* identities = ios::GetChromeBrowserProvider()
                            ->GetChromeIdentityService()
                            ->GetAllIdentitiesSortedForDisplay();
  if ([identities count]) {
    for (NSUInteger i = 0; i < [identities count]; ++i) {
      [model addItem:[self accountItemForIdentity:identities[i]
                                          checked:(i == 0)]
          toSectionWithIdentifier:SectionIdentifierAccounts];
    }
    [model addSectionWithIdentifier:SectionIdentifierAddAccount];
    [model addItem:[self addAccountItem]
        toSectionWithIdentifier:SectionIdentifierAddAccount];
  }
}

- (CollectionViewItem*)titleItem {
  // TODO(crbug.com/662549) : Rename FooterItem to be used as regular item.
  CollectionViewFooterItem* item =
      [[CollectionViewFooterItem alloc] initWithType:ItemTypeTitle];
  item.text =
      l10n_util::GetNSString(IDS_IOS_ACCOUNT_CONSISTENCY_SETUP_DESCRIPTION);
  return item;
}

- (CollectionViewItem*)accountItemForIdentity:(ChromeIdentity*)identity
                                      checked:(BOOL)isChecked {
  CollectionViewAccountItem* item =
      [[CollectionViewAccountItem alloc] initWithType:ItemTypeAccount];
  [self updateAccountItem:item withIdentity:identity];
  if (isChecked) {
    item.accessoryType = MDCCollectionViewCellAccessoryCheckmark;
  }
  return item;
}

- (void)updateAccountItem:(CollectionViewAccountItem*)item
             withIdentity:(ChromeIdentity*)identity {
  item.image = [_avatarCache resizedAvatarForIdentity:identity];
  item.text = identity.userEmail;
  item.chromeIdentity = identity;
}

- (CollectionViewItem*)addAccountItem {
  CollectionViewAccountItem* item =
      [[CollectionViewAccountItem alloc] initWithType:ItemTypeAddAccount];
  item.text = l10n_util::GetNSString(
      IDS_IOS_ACCOUNT_CONSISTENCY_SETUP_ADD_ACCOUNT_BUTTON);
  item.image = [UIImage imageNamed:@"settings_accounts_add_account"];
  return item;
}

- (ChromeIdentity*)selectedIdentity {
  NSArray* accountItems = [self.collectionViewModel
      itemsInSectionWithIdentifier:SectionIdentifierAccounts];
  for (CollectionViewAccountItem* accountItem in accountItems) {
    if (accountItem.accessoryType == MDCCollectionViewCellAccessoryCheckmark) {
      return accountItem.chromeIdentity;
    }
  }
  return nil;
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];
  if (item.type == ItemTypeAccount) {
    CollectionViewAccountItem* selectedAccountItem =
        base::mac::ObjCCastStrict<CollectionViewAccountItem>(item);
    // TODO(crbug.com/631486) : Checkmark animation.
    selectedAccountItem.accessoryType = MDCCollectionViewCellAccessoryCheckmark;

    NSMutableArray<CollectionViewItem*>* reloadItems =
        [[NSMutableArray alloc] init];
    [reloadItems addObject:selectedAccountItem];

    // Uncheck all the other account items.
    NSArray* accountItems = [self.collectionViewModel
        itemsInSectionWithIdentifier:SectionIdentifierAccounts];
    for (CollectionViewAccountItem* accountItem in accountItems) {
      if (accountItem != selectedAccountItem &&
          accountItem.accessoryType != MDCCollectionViewCellAccessoryNone) {
        // TODO(crbug.com/631486) : Checkmark animation.
        accountItem.accessoryType = MDCCollectionViewCellAccessoryNone;
        [reloadItems addObject:accountItem];
      }
    }
    [self reconfigureCellsForItems:reloadItems];
  } else if (item.type == ItemTypeAddAccount) {
    [self.delegate accountSelectorControllerDidSelectAddAccount:self];
  }
}

#pragma mark - ChromeIdentityServiceObserver

- (void)identityListChanged {
  ChromeIdentity* selectedIdentity = [self selectedIdentity];
  [self loadModel];
  [self.collectionView reloadData];

  // Reselect the identity.
  if (!selectedIdentity) {
    return;
  }
  NSArray* accountItems = [self.collectionViewModel
      itemsInSectionWithIdentifier:SectionIdentifierAccounts];
  for (CollectionViewAccountItem* accountItem in accountItems) {
    if ([accountItem.chromeIdentity.gaiaID
            isEqualToString:selectedIdentity.gaiaID]) {
      accountItem.accessoryType = MDCCollectionViewCellAccessoryCheckmark;
    } else {
      accountItem.accessoryType = MDCCollectionViewCellAccessoryNone;
    }
  }
}

- (void)chromeIdentityServiceWillBeDestroyed {
  _identityServiceObserver.reset();
}

#pragma mark UICollectionViewDataSource

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  MDCCollectionViewCell* cell =
      [super collectionView:collectionView cellForItemAtIndexPath:indexPath];
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];

  if (item.type == ItemTypeAccount || item.type == ItemTypeAddAccount) {
    CollectionViewAccountCell* accountCell =
        base::mac::ObjCCastStrict<CollectionViewAccountCell>(cell);
    accountCell.textLabel.font = [MDCTypography body1Font];
    accountCell.textLabel.textColor = [[MDCPalette greyPalette] tint900];
  } else if (item.type == ItemTypeTitle) {
    CollectionViewFooterCell* titleCell =
        base::mac::ObjCCastStrict<CollectionViewFooterCell>(cell);
    titleCell.textLabel.font = [MDCTypography body1Font];
    titleCell.textLabel.textColor = [[MDCPalette greyPalette] tint900];
    titleCell.horizontalPadding = 16;
  }
  cell.shouldHideSeparator = YES;
  return cell;
}

#pragma mark - MDCCollectionViewStylingDelegate

- (CGFloat)collectionView:(UICollectionView*)collectionView
    cellHeightAtIndexPath:(NSIndexPath*)indexPath {
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];

  if (item.type == ItemTypeTitle) {
    return [MDCCollectionViewCell
        cr_preferredHeightForWidth:CGRectGetWidth(collectionView.bounds)
                           forItem:item];
  }
  return MDCCellDefaultTwoLineHeight;
}

- (BOOL)collectionView:(UICollectionView*)collectionView
    hidesInkViewAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  return (itemType == ItemTypeTitle);
}

- (BOOL)collectionView:(nonnull UICollectionView*)collectionView
    shouldHideItemBackgroundAtIndexPath:(nonnull NSIndexPath*)indexPath {
  return YES;
}

@end
