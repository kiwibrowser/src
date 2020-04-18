// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_mediator.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/chrome_browser_provider_observer_bridge.h"
#import "ios/chrome/browser/signin/chrome_identity_service_observer_bridge.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_add_account_item.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_header_item.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_item.h"
#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_view_controller.h"
#import "ios/chrome/browser/ui/table_view/table_view_model.h"
#import "ios/public/provider/chrome/browser/signin/chrome_identity.h"
#include "ios/public/provider/chrome/browser/signin/chrome_identity_service.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  IdentitiesSectionIdentifier = kSectionIdentifierEnumZero,
  AddAccountSectionIdentifier,
};

typedef NS_ENUM(NSInteger, ItemType) {
  IdentityItemType = kItemTypeEnumZero,
  AddAccountItemType,
};

}  // namespace

@interface IdentityChooserMediator ()<ChromeIdentityServiceObserver,
                                      ChromeBrowserProviderObserver> {
  std::unique_ptr<ChromeIdentityServiceObserverBridge> _identityServiceObserver;
  std::unique_ptr<ChromeBrowserProviderObserverBridge> _browserProviderObserver;
}

// Gets the Chrome identity service.
@property(nonatomic, assign, readonly)
    ios::ChromeIdentityService* chromeIdentityService;

@end

@implementation IdentityChooserMediator

@synthesize identityChooserViewController = _identityChooserViewController;
@synthesize selectedIdentity = _selectedIdentity;

- (void)start {
  [self.identityChooserViewController loadModel];
  _identityServiceObserver =
      std::make_unique<ChromeIdentityServiceObserverBridge>(self);
  _browserProviderObserver =
      std::make_unique<ChromeBrowserProviderObserverBridge>(self);
  TableViewModel* tableViewModel =
      self.identityChooserViewController.tableViewModel;
  [self loadIdentitySection];
  [tableViewModel addSectionWithIdentifier:AddAccountSectionIdentifier];
  IdentityChooserAddAccountItem* addAccountItem =
      [[IdentityChooserAddAccountItem alloc] initWithType:AddAccountItemType];
  [tableViewModel addItem:addAccountItem
      toSectionWithIdentifier:AddAccountSectionIdentifier];
  [self.identityChooserViewController.tableView reloadData];
}

- (void)setSelectedIdentity:(ChromeIdentity*)selectedIdentity {
  if (_selectedIdentity == selectedIdentity)
    return;
  IdentityChooserItem* previousSelectedItem =
      [self identityChooserItemWithGaiaID:self.selectedIdentity.gaiaID];
  if (previousSelectedItem) {
    previousSelectedItem.selected = NO;
    [self.identityChooserViewController
        reconfigureCellsForItems:@[ previousSelectedItem ]];
  }
  _selectedIdentity = selectedIdentity;
  IdentityChooserItem* selectedItem =
      [self identityChooserItemWithGaiaID:self.selectedIdentity.gaiaID];
  DCHECK(selectedItem);
  selectedItem.selected = YES;
  [self.identityChooserViewController
      reconfigureCellsForItems:@[ selectedItem ]];
}

#pragma mark - Private

// Returns an IdentityChooserItem based on a gaia ID.
- (IdentityChooserItem*)identityChooserItemWithGaiaID:(NSString*)gaiaID {
  for (IdentityChooserItem* item in
       [self.identityChooserViewController.tableViewModel
           itemsInSectionWithIdentifier:IdentitiesSectionIdentifier]) {
    if ([item.gaiaID isEqualToString:gaiaID])
      return item;
  }
  return nil;
}

// Creates the identity section with its header item, and all the identity items
// based on the ChromeIdentity.
- (void)loadIdentitySection {
  TableViewModel* tableViewModel =
      self.identityChooserViewController.tableViewModel;
  DCHECK(![tableViewModel
      hasSectionForSectionIdentifier:IdentitiesSectionIdentifier]);
  // Create the section.
  [tableViewModel insertSectionWithIdentifier:IdentitiesSectionIdentifier
                                      atIndex:0];
  // Create the header item.
  [tableViewModel setHeader:[[IdentityChooserHeaderItem alloc] init]
      forSectionWithIdentifier:IdentitiesSectionIdentifier];
  // Create all the identity items.
  NSArray<ChromeIdentity*>* identities =
      self.chromeIdentityService->GetAllIdentitiesSortedForDisplay();
  for (ChromeIdentity* identity in identities) {
    IdentityChooserItem* item =
        [[IdentityChooserItem alloc] initWithType:IdentityItemType];
    [self updateIdentityChooserItem:item withChromeIdentity:identity];
    [self.identityChooserViewController.tableViewModel
                        addItem:item
        toSectionWithIdentifier:IdentitiesSectionIdentifier];
  }
}

// Updates an IdentityChooserItem based on a ChromeIdentity.
- (void)updateIdentityChooserItem:(IdentityChooserItem*)item
               withChromeIdentity:(ChromeIdentity*)identity {
  item.gaiaID = identity.gaiaID;
  item.name = identity.userFullName;
  item.email = identity.userEmail;
  item.selected =
      [self.selectedIdentity.gaiaID isEqualToString:identity.gaiaID];
  __weak __typeof(self) weakSelf = self;
  ios::GetAvatarCallback callback = ^(UIImage* identityAvatar) {
    if (![weakSelf.identityChooserViewController.tableViewModel hasItem:item]) {
      // Make sure the item is still displayed.
      return;
    }
    item.avatar = identityAvatar;
    [weakSelf.identityChooserViewController reconfigureCellsForItems:@[ item ]];
  };
  self.chromeIdentityService->GetAvatarForIdentity(identity, callback);
}

// Getter for the Chrome identity service.
- (ios::ChromeIdentityService*)chromeIdentityService {
  return ios::GetChromeBrowserProvider()->GetChromeIdentityService();
}

#pragma mark - IdentityChooserViewControllerSelectionDelegate

- (void)identityChooserViewController:
            (IdentityChooserViewController*)viewController
         didSelectIdentityAtIndexPath:(NSIndexPath*)indexPath {
  DCHECK_EQ(viewController, self.identityChooserViewController);
  DCHECK_EQ(0, indexPath.section);
  ListItem* item = [self.identityChooserViewController.tableViewModel
      itemAtIndexPath:indexPath];
  IdentityChooserItem* identityChooserItem =
      base::mac::ObjCCastStrict<IdentityChooserItem>(item);
  DCHECK(identityChooserItem);
  ChromeIdentity* identity = self.chromeIdentityService->GetIdentityWithGaiaID(
      base::SysNSStringToUTF8(identityChooserItem.gaiaID));
  DCHECK(identity);
  self.selectedIdentity = identity;
}

#pragma mark - ChromeIdentityServiceObserver

- (void)identityListChanged {
  TableViewModel* tableViewModel =
      self.identityChooserViewController.tableViewModel;
  [tableViewModel removeSectionWithIdentifier:IdentitiesSectionIdentifier];
  [self loadIdentitySection];
  [self.identityChooserViewController.tableView reloadData];
  // Updates the selection.
  NSArray* allIdentities =
      self.chromeIdentityService->GetAllIdentitiesSortedForDisplay();
  if (![allIdentities containsObject:self.selectedIdentity]) {
    self.selectedIdentity = allIdentities[0];
  }
}

- (void)profileUpdate:(ChromeIdentity*)identity {
  IdentityChooserItem* item =
      [self identityChooserItemWithGaiaID:identity.gaiaID];
  [self updateIdentityChooserItem:item withChromeIdentity:identity];
}

- (void)chromeIdentityServiceWillBeDestroyed {
  _identityServiceObserver.reset();
}

@end
