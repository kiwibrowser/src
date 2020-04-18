// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/about_chrome_collection_view_controller.h"

#import "base/ios/block_types.h"
#include "base/logging.h"
#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/version_info/version_info.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/settings/cells/version_item.h"
#import "ios/chrome/browser/ui/settings/settings_utils.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/common/channel_info.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"
#import "ios/third_party/material_components_ios/src/components/Snackbar/src/MaterialSnackbar.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierLinks = kSectionIdentifierEnumZero,
  SectionIdentifierFooter,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeLinksCredits = kItemTypeEnumZero,
  ItemTypeLinksTerms,
  ItemTypeLinksPrivacy,
  ItemTypeVersion,
};

}  // namespace

@implementation AboutChromeCollectionViewController

#pragma mark Initialization

- (instancetype)init {
  UICollectionViewLayout* layout = [[MDCCollectionViewFlowLayout alloc] init];
  self =
      [super initWithLayout:layout style:CollectionViewControllerStyleAppBar];
  if (self) {
    self.title = l10n_util::GetNSString(IDS_IOS_ABOUT_PRODUCT_NAME);
    // TODO(crbug.com/764578): -loadModel should not be called from
    // initializer. A possible fix is to move this call to -viewDidLoad.
    [self loadModel];
  }
  return self;
}

#pragma mark SettingsRootCollectionViewController

- (void)loadModel {
  [super loadModel];
  CollectionViewModel* model = self.collectionViewModel;

  [model addSectionWithIdentifier:SectionIdentifierLinks];

  CollectionViewTextItem* credits =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeLinksCredits];
  credits.text = l10n_util::GetNSString(IDS_IOS_OPEN_SOURCE_LICENSES);
  credits.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  credits.accessibilityTraits = UIAccessibilityTraitButton;
  [model addItem:credits toSectionWithIdentifier:SectionIdentifierLinks];

  CollectionViewTextItem* terms =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeLinksTerms];
  terms.text = l10n_util::GetNSString(IDS_IOS_TERMS_OF_SERVICE);
  terms.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  terms.accessibilityTraits = UIAccessibilityTraitButton;
  [model addItem:terms toSectionWithIdentifier:SectionIdentifierLinks];

  CollectionViewTextItem* privacy =
      [[CollectionViewTextItem alloc] initWithType:ItemTypeLinksPrivacy];
  privacy.text = l10n_util::GetNSString(IDS_IOS_PRIVACY_POLICY);
  privacy.accessoryType = MDCCollectionViewCellAccessoryDisclosureIndicator;
  privacy.accessibilityTraits = UIAccessibilityTraitButton;
  [model addItem:privacy toSectionWithIdentifier:SectionIdentifierLinks];

  [model addSectionWithIdentifier:SectionIdentifierFooter];

  VersionItem* version = [[VersionItem alloc] initWithType:ItemTypeVersion];
  version.text = [self versionDescriptionString];
  version.accessibilityTraits = UIAccessibilityTraitButton;
  [model addItem:version toSectionWithIdentifier:SectionIdentifierFooter];
}

#pragma mark UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];
  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (itemType) {
    case ItemTypeLinksCredits:
      [self openURL:GURL(kChromeUICreditsURL)];
      break;
    case ItemTypeLinksTerms:
      [self openURL:GURL(kChromeUITermsURL)];
      break;
    case ItemTypeLinksPrivacy:
      [self openURL:GURL(l10n_util::GetStringUTF8(IDS_IOS_PRIVACY_POLICY_URL))];
      break;
    case ItemTypeVersion:
      [self copyVersionToPasteboard];
      break;
    default:
      NOTREACHED();
      break;
  }
}

#pragma mark MDCCollectionViewStylingDelegate

// MDCCollectionViewStylingDelegate protocol is implemented so that the version
// cell has an invisible background.
- (MDCCollectionViewCellStyle)collectionView:(UICollectionView*)collectionView
                         cellStyleForSection:(NSInteger)section {
  NSInteger sectionIdentifier =
      [self.collectionViewModel sectionIdentifierForSection:section];
  switch (sectionIdentifier) {
    case SectionIdentifierFooter:
      return MDCCollectionViewCellStyleDefault;
    default:
      return self.styler.cellStyle;
  }
}

- (BOOL)collectionView:(UICollectionView*)collectionView
    shouldHideItemBackgroundAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger sectionIdentifier =
      [self.collectionViewModel sectionIdentifierForSection:indexPath.section];
  switch (sectionIdentifier) {
    case SectionIdentifierFooter:
      return YES;
    default:
      return NO;
  }
}

#pragma mark Private methods

- (void)openURL:(GURL)URL {
  BlockToOpenURL(self, self.dispatcher)(URL);
}

- (void)copyVersionToPasteboard {
  [[UIPasteboard generalPasteboard] setString:[self versionOnlyString]];
  TriggerHapticFeedbackForNotification(UINotificationFeedbackTypeSuccess);
  NSString* messageText = l10n_util::GetNSString(IDS_IOS_VERSION_COPIED);
  MDCSnackbarMessage* message =
      [MDCSnackbarMessage messageWithText:messageText];
  message.category = @"version copied";
  [MDCSnackbarManager showMessage:message];
}

- (std::string)versionString {
  std::string versionString = version_info::GetVersionNumber();
  std::string versionStringModifier = GetChannelString();
  if (!versionStringModifier.empty()) {
    versionString = versionString + " " + versionStringModifier;
  }
  return versionString;
}

- (NSString*)versionDescriptionString {
  return l10n_util::GetNSStringF(IDS_IOS_VERSION,
                                 base::UTF8ToUTF16([self versionString]));
}

- (NSString*)versionOnlyString {
  return base::SysUTF8ToNSString([self versionString]);
}

@end
