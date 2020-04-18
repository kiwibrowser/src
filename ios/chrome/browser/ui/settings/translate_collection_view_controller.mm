// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/translate_collection_view_controller.h"

#import <Foundation/Foundation.h>
#include <memory>

#include "base/mac/foundation_util.h"
#include "components/google/core/browser/google_util.h"
#include "components/prefs/pref_member.h"
#include "components/prefs/pref_service.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "ios/chrome/browser/application_context.h"
#import "ios/chrome/browser/translate/chrome_ios_translate_client.h"
#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/settings/settings_utils.h"
#import "ios/chrome/browser/ui/settings/utils/pref_backed_boolean.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Snackbar/src/MaterialSnackbar.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierTranslate = kSectionIdentifierEnumZero,
  SectionIdentifierFooter,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeTranslate = kItemTypeEnumZero,
  ItemTypeResetTranslate,
  ItemTypeFooter,
};

const char kTranslateLearnMoreUrl[] =
    "https://support.google.com/chrome/answer/3214105?p=mobile_translate";
NSString* const kTranslateSettingsCategory = @"ChromeTranslateSettings";

}  // namespace

@interface TranslateCollectionViewController ()<BooleanObserver> {
  // Profile preferences.
  PrefService* _prefs;  // weak
  PrefBackedBoolean* _translationEnabled;
  // The item related to the switch for the translation setting.
  CollectionViewSwitchItem* _translationItem;
}

@end

@implementation TranslateCollectionViewController

#pragma mark - Initialization

- (instancetype)initWithPrefs:(PrefService*)prefs {
  DCHECK(prefs);
  UICollectionViewLayout* layout = [[MDCCollectionViewFlowLayout alloc] init];
  self =
      [super initWithLayout:layout style:CollectionViewControllerStyleAppBar];
  if (self) {
    self.title = l10n_util::GetNSString(IDS_IOS_TRANSLATE_SETTING);
    self.collectionViewAccessibilityIdentifier =
        @"translate_settings_view_controller";
    _prefs = prefs;
    _translationEnabled = [[PrefBackedBoolean alloc]
        initWithPrefService:_prefs
                   prefName:prefs::kOfferTranslateEnabled];
    [_translationEnabled setObserver:self];
    // TODO(crbug.com/764578): -loadModel should not be called from
    // initializer. A possible fix is to move this call to -viewDidLoad.
    [self loadModel];
  }
  return self;
}

- (void)dealloc {
  [_translationEnabled setObserver:nil];
}

#pragma mark - SettingsRootCollectionViewController

- (void)loadModel {
  [super loadModel];
  CollectionViewModel* model = self.collectionViewModel;

  // Translate Section
  [model addSectionWithIdentifier:SectionIdentifierTranslate];
  _translationItem =
      [[CollectionViewSwitchItem alloc] initWithType:ItemTypeTranslate];
  _translationItem.text = l10n_util::GetNSString(IDS_IOS_TRANSLATE_SETTING);
  _translationItem.on = [_translationEnabled value];
  [model addItem:_translationItem
      toSectionWithIdentifier:SectionIdentifierTranslate];

  CollectionViewTextItem* resetTranslate =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeResetTranslate];
  resetTranslate.text = l10n_util::GetNSString(IDS_IOS_TRANSLATE_SETTING_RESET);
  resetTranslate.accessibilityTraits |= UIAccessibilityTraitButton;
  resetTranslate.textFont = [MDCTypography body2Font];
  [model addItem:resetTranslate
      toSectionWithIdentifier:SectionIdentifierTranslate];

  // Footer Section
  [model addSectionWithIdentifier:SectionIdentifierFooter];
  CollectionViewFooterItem* footer =
      [[CollectionViewFooterItem alloc] initWithType:ItemTypeFooter];
  footer.text = l10n_util::GetNSString(IDS_IOS_TRANSLATE_SETTING_DESCRIPTION);
  footer.linkURL = google_util::AppendGoogleLocaleParam(
      GURL(kTranslateLearnMoreUrl),
      GetApplicationContext()->GetApplicationLocale());
  footer.linkDelegate = self;
  [model addItem:footer toSectionWithIdentifier:SectionIdentifierFooter];
}

#pragma mark UICollectionViewDataSource

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  UICollectionViewCell* cell =
      [super collectionView:collectionView cellForItemAtIndexPath:indexPath];
  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];

  switch (itemType) {
    case ItemTypeTranslate: {
      CollectionViewSwitchCell* switchCell =
          base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
      [switchCell.switchView addTarget:self
                                action:@selector(translateToggled:)
                      forControlEvents:UIControlEventValueChanged];
      break;
    }
    default:
      break;
  }

  return cell;
}

#pragma mark UICollectionViewDelegate
- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];

  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];

  if (itemType == ItemTypeResetTranslate) {
    std::unique_ptr<translate::TranslatePrefs> translatePrefs(
        ChromeIOSTranslateClient::CreateTranslatePrefs(_prefs));
    translatePrefs->ResetToDefaults();
    TriggerHapticFeedbackForNotification(UINotificationFeedbackTypeSuccess);
    NSString* messageText =
        l10n_util::GetNSString(IDS_IOS_TRANSLATE_SETTING_RESET_NOTIFICATION);
    MDCSnackbarMessage* message =
        [MDCSnackbarMessage messageWithText:messageText];
    message.category = kTranslateSettingsCategory;
    [MDCSnackbarManager showMessage:message];
  }
}

#pragma mark MDCCollectionViewStylingDelegate

- (MDCCollectionViewCellStyle)collectionView:(UICollectionView*)collectionView
                         cellStyleForSection:(NSInteger)section {
  NSInteger sectionIdentifier =
      [self.collectionViewModel sectionIdentifierForSection:section];

  if (sectionIdentifier == SectionIdentifierFooter) {
    return MDCCollectionViewCellStyleDefault;
  }

  return self.styler.cellStyle;
}

- (BOOL)collectionView:(UICollectionView*)collectionView
    shouldHideItemBackgroundAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger sectionIdentifier =
      [self.collectionViewModel sectionIdentifierForSection:indexPath.section];

  if (sectionIdentifier == SectionIdentifierFooter) {
    return YES;
  }

  return NO;
}

- (CGFloat)collectionView:(UICollectionView*)collectionView
    cellHeightAtIndexPath:(NSIndexPath*)indexPath {
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];

  if (item.type == ItemTypeFooter)
    return [MDCCollectionViewCell
        cr_preferredHeightForWidth:CGRectGetWidth(collectionView.bounds)
                           forItem:item];
  return MDCCellDefaultOneLineHeight;
}

- (BOOL)collectionView:(UICollectionView*)collectionView
    hidesInkViewAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (type) {
    case ItemTypeFooter:
    case ItemTypeTranslate:
      return YES;
    default:
      return NO;
  }
}

#pragma mark - BooleanObserver

- (void)booleanDidChange:(id<ObservableBoolean>)observableBoolean {
  DCHECK_EQ(observableBoolean, _translationEnabled);

  // Update the item.
  _translationItem.on = [_translationEnabled value];

  // Update the cell.
  [self reconfigureCellsForItems:@[ _translationItem ]];
}

#pragma mark - Actions

- (void)translateToggled:(id)sender {
  NSIndexPath* switchPath = [self.collectionViewModel
      indexPathForItemType:ItemTypeTranslate
         sectionIdentifier:SectionIdentifierTranslate];

  CollectionViewSwitchItem* switchItem =
      base::mac::ObjCCastStrict<CollectionViewSwitchItem>(
          [self.collectionViewModel itemAtIndexPath:switchPath]);
  CollectionViewSwitchCell* switchCell =
      base::mac::ObjCCastStrict<CollectionViewSwitchCell>(
          [self.collectionView cellForItemAtIndexPath:switchPath]);

  DCHECK_EQ(switchCell.switchView, sender);
  BOOL isOn = switchCell.switchView.isOn;
  switchItem.on = isOn;
  [_translationEnabled setValue:isOn];
}

@end
