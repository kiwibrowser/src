// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/bandwidth_management_collection_view_controller.h"

#include "base/mac/foundation_util.h"
#import "components/prefs/ios/pref_observer_bridge.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_detail_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#import "ios/chrome/browser/ui/settings/dataplan_usage_collection_view_controller.h"
#import "ios/chrome/browser/ui/settings/settings_utils.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierActions = kSectionIdentifierEnumZero,
  SectionIdentifierFooter,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypePreload = kItemTypeEnumZero,
  ItemTypeFooter,
};

}  // namespace

@interface BandwidthManagementCollectionViewController ()<
    PrefObserverDelegate> {
  ios::ChromeBrowserState* _browserState;  // weak

  // Pref observer to track changes to prefs.
  std::unique_ptr<PrefObserverBridge> _prefObserverBridge;
  // Registrar for pref changes notifications.
  PrefChangeRegistrar _prefChangeRegistrarApplicationContext;

  // Updatable Items
  CollectionViewDetailItem* _preloadWebpagesDetailItem;
}

// Helpers to create collection view items.
- (CollectionViewItem*)preloadWebpagesItem;
- (CollectionViewItem*)footerItem;
@end

@implementation BandwidthManagementCollectionViewController

- (instancetype)initWithBrowserState:(ios::ChromeBrowserState*)browserState {
  UICollectionViewLayout* layout = [[MDCCollectionViewFlowLayout alloc] init];
  self =
      [super initWithLayout:layout style:CollectionViewControllerStyleAppBar];
  if (self) {
    self.title = l10n_util::GetNSString(IDS_IOS_BANDWIDTH_MANAGEMENT_SETTINGS);
    self.collectionViewAccessibilityIdentifier = @"Bandwidth Management";
    _browserState = browserState;

    _prefChangeRegistrarApplicationContext.Init(_browserState->GetPrefs());
    _prefObserverBridge.reset(new PrefObserverBridge(self));
    // Register to observe any changes on Perf backed values displayed by the
    // screen.
    _prefObserverBridge->ObserveChangesForPreference(
        prefs::kNetworkPredictionEnabled,
        &_prefChangeRegistrarApplicationContext);
    _prefObserverBridge->ObserveChangesForPreference(
        prefs::kNetworkPredictionWifiOnly,
        &_prefChangeRegistrarApplicationContext);

    // TODO(crbug.com/764578): -loadModel should not be called from
    // initializer. A possible fix is to move this call to -viewDidLoad.
    [self loadModel];
  }
  return self;
}

- (void)loadModel {
  [super loadModel];

  CollectionViewModel* model = self.collectionViewModel;
  [model addSectionWithIdentifier:SectionIdentifierActions];
  [model addItem:[self preloadWebpagesItem]
      toSectionWithIdentifier:SectionIdentifierActions];

  // The footer item must currently go into a separate section, to work around a
  // drawing bug in MDC.
  [model addSectionWithIdentifier:SectionIdentifierFooter];
  [model addItem:[self footerItem]
      toSectionWithIdentifier:SectionIdentifierFooter];
}

- (CollectionViewItem*)preloadWebpagesItem {
  NSString* detailText = [DataplanUsageCollectionViewController
      currentLabelForPreference:_browserState->GetPrefs()
                       basePref:prefs::kNetworkPredictionEnabled
                       wifiPref:prefs::kNetworkPredictionWifiOnly];
  _preloadWebpagesDetailItem =
      [[CollectionViewDetailItem alloc] initWithType:ItemTypePreload];

  _preloadWebpagesDetailItem.text =
      l10n_util::GetNSString(IDS_IOS_OPTIONS_PRELOAD_WEBPAGES);
  _preloadWebpagesDetailItem.detailText = detailText;
  _preloadWebpagesDetailItem.accessoryType =
      MDCCollectionViewCellAccessoryDisclosureIndicator;
  _preloadWebpagesDetailItem.accessibilityTraits |= UIAccessibilityTraitButton;
  return _preloadWebpagesDetailItem;
}

- (CollectionViewItem*)footerItem {
  CollectionViewFooterItem* item =
      [[CollectionViewFooterItem alloc] initWithType:ItemTypeFooter];

  item.text = l10n_util::GetNSString(
      IDS_IOS_BANDWIDTH_MANAGEMENT_DESCRIPTION_LEARN_MORE);
  item.linkURL =
      GURL(l10n_util::GetStringUTF8(IDS_IOS_BANDWIDTH_MANAGEMENT_LEARN_URL));
  item.linkDelegate = self;
  item.accessibilityTraits |= UIAccessibilityTraitButton;
  return item;
}

#pragma mark UICollectionViewDelegate

- (BOOL)collectionView:(UICollectionView*)collectionView
    shouldHighlightItemAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  if (type == ItemTypeFooter) {
    return NO;
  }
  return [super collectionView:collectionView
      shouldHighlightItemAtIndexPath:indexPath];
}

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  [super collectionView:collectionView didSelectItemAtIndexPath:indexPath];

  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  if (type == ItemTypePreload) {
    NSString* preloadTitle =
        l10n_util::GetNSString(IDS_IOS_OPTIONS_PRELOAD_WEBPAGES);
    UIViewController* controller =
        [[DataplanUsageCollectionViewController alloc]
            initWithPrefs:_browserState->GetPrefs()
                 basePref:prefs::kNetworkPredictionEnabled
                 wifiPref:prefs::kNetworkPredictionWifiOnly
                    title:preloadTitle];
    [self.navigationController pushViewController:controller animated:YES];
  }
}

#pragma mark - MDCCollectionViewStylingDelegate

- (MDCCollectionViewCellStyle)collectionView:(UICollectionView*)collectionView
                         cellStyleForSection:(NSInteger)section {
  NSInteger sectionIdentifier =
      [self.collectionViewModel sectionIdentifierForSection:section];
  switch (sectionIdentifier) {
    case SectionIdentifierFooter:
      // Display the Learn More footer in the default style with no "card" UI
      // and no section padding.
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
      // Display the Learn More footer without any background image or
      // shadowing.
      return YES;
    default:
      return NO;
  }
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

#pragma mark - PrefObserverDelegate

- (void)onPreferenceChanged:(const std::string&)preferenceName {
  if (preferenceName == prefs::kNetworkPredictionEnabled ||
      preferenceName == prefs::kNetworkPredictionWifiOnly) {
    NSString* detailText = [DataplanUsageCollectionViewController
        currentLabelForPreference:_browserState->GetPrefs()
                         basePref:prefs::kNetworkPredictionEnabled
                         wifiPref:prefs::kNetworkPredictionWifiOnly];

    _preloadWebpagesDetailItem.detailText = detailText;

    [self reconfigureCellsForItems:@[ _preloadWebpagesDetailItem ]];
  }
}

@end
