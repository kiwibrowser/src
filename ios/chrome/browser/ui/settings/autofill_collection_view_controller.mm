// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/autofill_collection_view_controller.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#import "components/autofill/ios/browser/credit_card_util.h"
#import "components/autofill/ios/browser/personal_data_manager_observer_bridge.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/settings/autofill_credit_card_edit_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/autofill_profile_edit_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/cells/autofill_data_item.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierSwitches = kSectionIdentifierEnumZero,
  SectionIdentifierProfiles,
  SectionIdentifierCards,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeAutofillSwitch = kItemTypeEnumZero,
  ItemTypeWalletSwitch,
  ItemTypeAddress,
  ItemTypeCard,
  ItemTypeHeader,
};

}  // namespace

#pragma mark - AutofillCollectionViewController

@interface AutofillCollectionViewController ()<
    PersonalDataManagerObserverBridgeDelegate> {
  std::string _locale;  // User locale.
  autofill::PersonalDataManager* _personalDataManager;

  ios::ChromeBrowserState* _browserState;
  std::unique_ptr<autofill::PersonalDataManagerObserverBridge> _observer;
  BOOL _deletionInProgress;

  // Writing user-initiated switch state changes to the pref service results in
  // an observer callback, which handles general data updates with a reloadData.
  // It is better to handle user-initiated changes with more specific actions
  // such as inserting or removing items/sections. This boolean is used to
  // stop the observer callback from acting on user-initiated changes.
  BOOL _userInteractionInProgress;
}

@end

@implementation AutofillCollectionViewController

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState {
  DCHECK(browserState);
  UICollectionViewLayout* layout = [[MDCCollectionViewFlowLayout alloc] init];
  self =
      [super initWithLayout:layout style:CollectionViewControllerStyleAppBar];
  if (self) {
    self.collectionViewAccessibilityIdentifier = @"kAutofillCollectionViewId";
    self.title = l10n_util::GetNSString(IDS_IOS_AUTOFILL);
    self.shouldHideDoneButton = YES;
    _browserState = browserState;
    _locale = GetApplicationContext()->GetApplicationLocale();
    _personalDataManager =
        autofill::PersonalDataManagerFactory::GetForBrowserState(_browserState);
    _observer.reset(new autofill::PersonalDataManagerObserverBridge(self));
    _personalDataManager->AddObserver(_observer.get());

    // TODO(crbug.com/764578): -updateEditButton and -loadModel should not be
    // called from initializer.
    [self updateEditButton];
    [self loadModel];
  }
  return self;
}

- (void)dealloc {
  _personalDataManager->RemoveObserver(_observer.get());
}

#pragma mark - CollectionViewController

- (void)loadModel {
  [super loadModel];
  CollectionViewModel* model = self.collectionViewModel;

  [model addSectionWithIdentifier:SectionIdentifierSwitches];
  [model addItem:[self autofillSwitchItem]
      toSectionWithIdentifier:SectionIdentifierSwitches];

  if ([self isAutofillEnabled]) {
    [model addItem:[self walletSwitchItem]
        toSectionWithIdentifier:SectionIdentifierSwitches];

    [self populateProfileSection];
    [self populateCardSection];
  }
}

#pragma mark - LoadModel Helpers

// Populates profile section using personalDataManager.
- (void)populateProfileSection {
  CollectionViewModel* model = self.collectionViewModel;
  const std::vector<autofill::AutofillProfile*> autofillProfiles =
      _personalDataManager->GetProfiles();
  if (!autofillProfiles.empty()) {
    [model addSectionWithIdentifier:SectionIdentifierProfiles];
    [model setHeader:[self profileSectionHeader]
        forSectionWithIdentifier:SectionIdentifierProfiles];
    for (autofill::AutofillProfile* autofillProfile : autofillProfiles) {
      DCHECK(autofillProfile);
      [model addItem:[self itemForProfile:*autofillProfile]
          toSectionWithIdentifier:SectionIdentifierProfiles];
    }
  }
}

// Populates card section using personalDataManager.
- (void)populateCardSection {
  CollectionViewModel* model = self.collectionViewModel;
  const std::vector<autofill::CreditCard*>& creditCards =
      _personalDataManager->GetCreditCards();
  if (!creditCards.empty()) {
    [model addSectionWithIdentifier:SectionIdentifierCards];
    [model setHeader:[self cardSectionHeader]
        forSectionWithIdentifier:SectionIdentifierCards];
    for (autofill::CreditCard* creditCard : creditCards) {
      DCHECK(creditCard);
      [model addItem:[self itemForCreditCard:*creditCard]
          toSectionWithIdentifier:SectionIdentifierCards];
    }
  }
}

- (CollectionViewItem*)autofillSwitchItem {
  CollectionViewSwitchItem* switchItem =
      [[CollectionViewSwitchItem alloc] initWithType:ItemTypeAutofillSwitch];
  switchItem.text = l10n_util::GetNSString(IDS_IOS_AUTOFILL);
  switchItem.on = [self isAutofillEnabled];
  switchItem.accessibilityIdentifier = @"autofillItem_switch";
  return switchItem;
}

- (CollectionViewItem*)walletSwitchItem {
  CollectionViewSwitchItem* switchItem =
      [[CollectionViewSwitchItem alloc] initWithType:ItemTypeWalletSwitch];
  switchItem.text = l10n_util::GetNSString(IDS_IOS_AUTOFILL_USE_WALLET_DATA);
  switchItem.on = [self isWalletEnabled];
  switchItem.accessibilityIdentifier = @"walletItem_switch";
  return switchItem;
}

- (CollectionViewItem*)profileSectionHeader {
  CollectionViewTextItem* header = [self genericHeader];
  header.text = l10n_util::GetNSString(IDS_IOS_AUTOFILL_ADDRESSES_GROUP_NAME);
  return header;
}

- (CollectionViewItem*)cardSectionHeader {
  CollectionViewTextItem* header = [self genericHeader];
  header.text = l10n_util::GetNSString(IDS_IOS_AUTOFILL_CREDITCARDS_GROUP_NAME);
  return header;
}

- (CollectionViewTextItem*)genericHeader {
  CollectionViewTextItem* header =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeHeader];
  header.textColor = [[MDCPalette greyPalette] tint500];
  return header;
}

- (CollectionViewItem*)itemForProfile:
    (const autofill::AutofillProfile&)autofillProfile {
  std::string guid(autofillProfile.guid());
  NSString* title = base::SysUTF16ToNSString(autofillProfile.GetInfo(
      autofill::AutofillType(autofill::NAME_FULL), _locale));
  NSString* subTitle = base::SysUTF16ToNSString(autofillProfile.GetInfo(
      autofill::AutofillType(autofill::ADDRESS_HOME_LINE1), _locale));
  bool isServerProfile = autofillProfile.record_type() ==
                         autofill::AutofillProfile::SERVER_PROFILE;

  AutofillDataItem* item =
      [[AutofillDataItem alloc] initWithType:ItemTypeAddress];
  item.text = title;
  item.leadingDetailText = subTitle;
  item.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  item.accessibilityIdentifier = title;
  item.GUID = guid;
  item.deletable = !isServerProfile;
  if (isServerProfile) {
    item.trailingDetailText =
        l10n_util::GetNSString(IDS_IOS_AUTOFILL_WALLET_SERVER_NAME);
  }
  return item;
}

- (CollectionViewItem*)itemForCreditCard:
    (const autofill::CreditCard&)creditCard {
  std::string guid(creditCard.guid());
  NSString* creditCardName = autofill::GetCreditCardName(creditCard, _locale);

  AutofillDataItem* item = [[AutofillDataItem alloc] initWithType:ItemTypeCard];
  item.text = creditCardName;
  item.leadingDetailText = autofill::GetCreditCardObfuscatedNumber(creditCard);
  item.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  item.accessibilityIdentifier = creditCardName;
  item.deletable = autofill::IsCreditCardLocal(creditCard);
  item.GUID = guid;
  if (![item isDeletable]) {
    item.trailingDetailText =
        l10n_util::GetNSString(IDS_IOS_AUTOFILL_WALLET_SERVER_NAME);
  }
  return item;
}

- (BOOL)localProfilesOrCreditCardsExist {
  return !_personalDataManager->GetProfiles().empty() ||
         !_personalDataManager->GetLocalCreditCards().empty();
}

#pragma mark - SettingsRootCollectionViewController

- (BOOL)shouldShowEditButton {
  return [self isAutofillEnabled];
}

- (BOOL)editButtonEnabled {
  return [self localProfilesOrCreditCardsExist];
}

#pragma mark - UICollectionViewDataSource

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  UICollectionViewCell* cell =
      [super collectionView:collectionView cellForItemAtIndexPath:indexPath];

  ItemType itemType = static_cast<ItemType>(
      [self.collectionViewModel itemTypeForIndexPath:indexPath]);

  if (itemType == ItemTypeAutofillSwitch) {
    CollectionViewSwitchCell* switchCell =
        base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
    [switchCell.switchView addTarget:self
                              action:@selector(autofillSwitchChanged:)
                    forControlEvents:UIControlEventValueChanged];
  } else if (itemType == ItemTypeWalletSwitch) {
    CollectionViewSwitchCell* switchCell =
        base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
    [switchCell.switchView addTarget:self
                              action:@selector(walletSwitchChanged:)
                    forControlEvents:UIControlEventValueChanged];
  }
  return cell;
}

#pragma mark - Switch Callbacks

- (void)autofillSwitchChanged:(UISwitch*)switchView {
  [self setSwitchItemOn:[switchView isOn] itemType:ItemTypeAutofillSwitch];
  _userInteractionInProgress = YES;
  [self setAutofillEnabled:[switchView isOn]];
  _userInteractionInProgress = NO;
  [self updateEditButton];

  // Avoid reference cycle in block.
  __weak AutofillCollectionViewController* weakSelf = self;
  [self.collectionView performBatchUpdates:^{
    // Obtain strong reference again.
    AutofillCollectionViewController* strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }

    if ([switchView isOn]) {
      [strongSelf insertWalletSwitchItem];
      [strongSelf insertProfileAndCardSections];
    } else {
      [strongSelf removeWalletSwitchItem];
      [strongSelf removeProfileAndCardSections];
    }
  }
                                completion:nil];
}

- (void)walletSwitchChanged:(UISwitch*)switchView {
  [self setSwitchItemOn:[switchView isOn] itemType:ItemTypeWalletSwitch];
  _userInteractionInProgress = YES;
  [self setWalletEnabled:[switchView isOn]];
  _userInteractionInProgress = NO;
}

#pragma mark - Switch Helpers

// Sets switchItem's state to |on|. It is important that there is only one item
// of |switchItemType| in SectionIdentifierSwitches.
- (void)setSwitchItemOn:(BOOL)on itemType:(ItemType)switchItemType {
  NSIndexPath* switchPath =
      [self.collectionViewModel indexPathForItemType:switchItemType
                                   sectionIdentifier:SectionIdentifierSwitches];
  CollectionViewSwitchItem* switchItem =
      base::mac::ObjCCastStrict<CollectionViewSwitchItem>(
          [self.collectionViewModel itemAtIndexPath:switchPath]);
  switchItem.on = on;
}

// Sets switchItem's enabled status to |enabled| and reconfigures the
// corresponding cell. It is important that there is no more than one item of
// |switchItemType| in SectionIdentifierSwitches.
- (void)setSwitchItemEnabled:(BOOL)enabled itemType:(ItemType)switchItemType {
  CollectionViewModel* model = self.collectionViewModel;

  if (![model hasItemForItemType:switchItemType
               sectionIdentifier:SectionIdentifierSwitches]) {
    return;
  }
  NSIndexPath* switchPath =
      [model indexPathForItemType:switchItemType
                sectionIdentifier:SectionIdentifierSwitches];
  CollectionViewSwitchItem* switchItem =
      base::mac::ObjCCastStrict<CollectionViewSwitchItem>(
          [model itemAtIndexPath:switchPath]);
  [switchItem setEnabled:enabled];
  [self reconfigureCellsForItems:@[ switchItem ]];
}

#pragma mark - Insert or Delete Items and Sections

- (void)insertWalletSwitchItem {
  CollectionViewModel* model = self.collectionViewModel;
  [model addItem:[self walletSwitchItem]
      toSectionWithIdentifier:SectionIdentifierSwitches];
  NSIndexPath* indexPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeWalletSwitch
                                   sectionIdentifier:SectionIdentifierSwitches];
  [self.collectionView insertItemsAtIndexPaths:@[ indexPath ]];
}

- (void)removeWalletSwitchItem {
  if (![self.collectionViewModel
          hasItemForItemType:ItemTypeWalletSwitch
           sectionIdentifier:SectionIdentifierSwitches]) {
    return;
  }
  NSIndexPath* indexPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeWalletSwitch
                                   sectionIdentifier:SectionIdentifierSwitches];
  [self.collectionViewModel removeItemWithType:ItemTypeWalletSwitch
                     fromSectionWithIdentifier:SectionIdentifierSwitches];
  [self.collectionView deleteItemsAtIndexPaths:@[ indexPath ]];
}

- (void)insertProfileAndCardSections {
  [self populateProfileSection];
  [self populateCardSection];
  NSIndexSet* sections = [self indexSetForExistingProfileAndCardSections];
  [self.collectionView insertSections:sections];
}

- (void)removeProfileAndCardSections {
  // It is important to build the section indexSet before removing sections.
  NSIndexSet* sections = [self indexSetForExistingProfileAndCardSections];
  if ([self.collectionViewModel
          hasSectionForSectionIdentifier:SectionIdentifierProfiles]) {
    [self.collectionViewModel
        removeSectionWithIdentifier:SectionIdentifierProfiles];
  }
  if ([self.collectionViewModel
          hasSectionForSectionIdentifier:SectionIdentifierCards]) {
    [self.collectionViewModel
        removeSectionWithIdentifier:SectionIdentifierCards];
  }
  [self.collectionView deleteSections:sections];
}

- (NSIndexSet*)indexSetForExistingProfileAndCardSections {
  NSMutableIndexSet* sections = [[NSMutableIndexSet alloc] init];
  if ([self.collectionViewModel
          hasSectionForSectionIdentifier:SectionIdentifierProfiles]) {
    [sections
        addIndex:[self.collectionViewModel
                     sectionForSectionIdentifier:SectionIdentifierProfiles]];
  }
  if ([self.collectionViewModel
          hasSectionForSectionIdentifier:SectionIdentifierCards]) {
    [sections addIndex:[self.collectionViewModel
                           sectionForSectionIdentifier:SectionIdentifierCards]];
  }
  return sections;
}

#pragma mark - MDCCollectionViewStylingDelegate

- (CGFloat)collectionView:(UICollectionView*)collectionView
    cellHeightAtIndexPath:(NSIndexPath*)indexPath {
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];
  if (item.type == ItemTypeAddress || item.type == ItemTypeCard ||
      item.type == ItemTypeWalletSwitch) {
    return [MDCCollectionViewCell
        cr_preferredHeightForWidth:CGRectGetWidth(collectionView.bounds)
                           forItem:item];
  }
  return MDCCellDefaultOneLineHeight;
}

- (BOOL)collectionView:(UICollectionView*)collectionView
    hidesInkViewAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (type) {
    case ItemTypeAutofillSwitch:
    case ItemTypeWalletSwitch:
      return YES;
    default:
      return NO;
  }
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];

  // Edit mode is the state where the user can select and delete entries. In
  // edit mode, selection is handled by the superclass. When not in edit mode
  // selection presents the editing controller for the selected entry.
  if ([self.editor isEditing]) {
    return;
  }

  CollectionViewModel* model = self.collectionViewModel;
  SettingsRootCollectionViewController* controller;
  switch ([model itemTypeForIndexPath:indexPath]) {
    case ItemTypeAddress: {
      const std::vector<autofill::AutofillProfile*> autofillProfiles =
          _personalDataManager->GetProfiles();
      controller = [AutofillProfileEditCollectionViewController
          controllerWithProfile:*autofillProfiles[indexPath.item]
            personalDataManager:_personalDataManager];
      break;
    }
    case ItemTypeCard: {
      const std::vector<autofill::CreditCard*>& creditCards =
          _personalDataManager->GetCreditCards();
      controller = [[AutofillCreditCardEditCollectionViewController alloc]
           initWithCreditCard:*creditCards[indexPath.item]
          personalDataManager:_personalDataManager];
      break;
    }
    default:
      break;
  }

  if (controller) {
    controller.dispatcher = self.dispatcher;
    [self.navigationController pushViewController:controller animated:YES];
  }
}

#pragma mark - MDCCollectionViewEditingDelegate

- (BOOL)collectionViewAllowsEditing:(UICollectionView*)collectionView {
  return YES;
}

- (void)collectionViewWillBeginEditing:(UICollectionView*)collectionView {
  [super collectionViewWillBeginEditing:collectionView];

  [self setSwitchItemEnabled:NO itemType:ItemTypeAutofillSwitch];
  [self setSwitchItemEnabled:NO itemType:ItemTypeWalletSwitch];
}

- (void)collectionViewWillEndEditing:(UICollectionView*)collectionView {
  [super collectionViewWillEndEditing:collectionView];

  [self setSwitchItemEnabled:YES itemType:ItemTypeAutofillSwitch];
  [self setSwitchItemEnabled:YES itemType:ItemTypeWalletSwitch];
}

- (BOOL)collectionView:(UICollectionView*)collectionView
    canEditItemAtIndexPath:(NSIndexPath*)indexPath {
  // Only autofill data cells are editable.
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];
  if ([item isKindOfClass:[AutofillDataItem class]]) {
    AutofillDataItem* autofillItem =
        base::mac::ObjCCastStrict<AutofillDataItem>(item);
    return [autofillItem isDeletable];
  }
  return NO;
}

- (void)collectionView:(UICollectionView*)collectionView
    willDeleteItemsAtIndexPaths:(NSArray*)indexPaths {
  _deletionInProgress = YES;
  for (NSIndexPath* indexPath in indexPaths) {
    AutofillDataItem* item = base::mac::ObjCCastStrict<AutofillDataItem>(
        [self.collectionViewModel itemAtIndexPath:indexPath]);
    _personalDataManager->RemoveByGUID([item GUID]);
  }
  // Must call super at the end of the child implementation.
  [super collectionView:collectionView willDeleteItemsAtIndexPaths:indexPaths];
}

- (void)collectionView:(UICollectionView*)collectionView
    didDeleteItemsAtIndexPaths:(NSArray*)indexPaths {
  // If there are no index paths, return early. This can happen if the user
  // presses the Delete button twice in quick succession.
  if (![indexPaths count])
    return;

  // TODO(crbug.com/650390) Generalize removing empty sections
  [self removeSectionIfEmptyForSectionWithIdentifier:SectionIdentifierProfiles];
  [self removeSectionIfEmptyForSectionWithIdentifier:SectionIdentifierCards];
}

// Remove the section from the model and collectionView if there are no more
// items in the section.
- (void)removeSectionIfEmptyForSectionWithIdentifier:
    (SectionIdentifier)sectionIdentifier {
  if (![self.collectionViewModel
          hasSectionForSectionIdentifier:sectionIdentifier]) {
    return;
  }
  NSInteger section =
      [self.collectionViewModel sectionForSectionIdentifier:sectionIdentifier];
  if ([self.collectionView numberOfItemsInSection:section] == 0) {
    // Avoid reference cycle in block.
    __weak AutofillCollectionViewController* weakSelf = self;
    [self.collectionView performBatchUpdates:^{
      // Obtain strong reference again.
      AutofillCollectionViewController* strongSelf = weakSelf;
      if (!strongSelf) {
        return;
      }

      // Remove section from model and collectionView.
      [[strongSelf collectionViewModel]
          removeSectionWithIdentifier:sectionIdentifier];
      [[strongSelf collectionView]
          deleteSections:[NSIndexSet indexSetWithIndex:section]];
    }
        completion:^(BOOL finished) {
          // Obtain strong reference again.
          AutofillCollectionViewController* strongSelf = weakSelf;
          if (!strongSelf) {
            return;
          }

          // Turn off edit mode if there is nothing to edit.
          if (![strongSelf localProfilesOrCreditCardsExist] &&
              [strongSelf.editor isEditing]) {
            [[strongSelf editor] setEditing:NO];
          }
          [strongSelf updateEditButton];
          strongSelf->_deletionInProgress = NO;
        }];
  }
}

#pragma mark PersonalDataManagerObserverBridgeDelegate

- (void)onPersonalDataChanged {
  // If the change is due to local editing, or if local editing is happening
  // concurrently, updates are handled by collection view editing callbacks.
  // Data is reloaded at the end of deletion to make sure entries are in sync.
  if (_deletionInProgress)
    return;

  // If the change is due to user-initiated switch state changes, updates
  // are handled by the switch callbacks.
  if (_userInteractionInProgress)
    return;

  if (![self localProfilesOrCreditCardsExist] && [self.editor isEditing]) {
    // Turn off edit mode if there exists nothing to edit.
    [self.editor setEditing:NO];
  }

  [self updateEditButton];
  [self reloadData];
}

#pragma mark - Pref Helpers

- (BOOL)isAutofillEnabled {
  return _browserState->GetPrefs()->GetBoolean(
      autofill::prefs::kAutofillEnabled);
}

- (void)setAutofillEnabled:(BOOL)isEnabled {
  _browserState->GetPrefs()->SetBoolean(autofill::prefs::kAutofillEnabled,
                                        isEnabled);
}

- (BOOL)isWalletEnabled {
  return _browserState->GetPrefs()->GetBoolean(
      autofill::prefs::kAutofillWalletImportEnabled);
}

- (void)setWalletEnabled:(BOOL)isEnabled {
  _browserState->GetPrefs()->SetBoolean(
      autofill::prefs::kAutofillWalletImportEnabled, isEnabled);
}

@end
