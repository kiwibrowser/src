// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/do_not_track_collection_view_controller.h"

#include "base/mac/foundation_util.h"
#include "components/google/core/browser/google_util.h"
#include "components/prefs/pref_member.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_switch_item.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_model.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierSwitch = kSectionIdentifierEnumZero,
  SectionIdentifierFooter,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeSwitch = kItemTypeEnumZero,
  ItemTypeFooter,
};

}  // namespace

@interface DoNotTrackCollectionViewController () {
  // Pref for whether or not 'Do Not Track' is enabled.
  BooleanPrefMember _doNotTrackEnabled;
}

// Returns the item to be used as the preference switch.
- (CollectionViewItem*)switchItem;

// Returns the item to be used as a footer.
- (CollectionViewItem*)footerItem;
@end

@implementation DoNotTrackCollectionViewController

#pragma mark - Initialization

- (instancetype)initWithPrefs:(PrefService*)prefs {
  UICollectionViewLayout* layout = [[MDCCollectionViewFlowLayout alloc] init];
  self =
      [super initWithLayout:layout style:CollectionViewControllerStyleAppBar];
  if (self) {
    self.title = l10n_util::GetNSString(IDS_IOS_OPTIONS_DO_NOT_TRACK_MOBILE);
    _doNotTrackEnabled.Init(prefs::kEnableDoNotTrack, prefs);
    self.collectionViewAccessibilityIdentifier = @"Do Not Track";
    // TODO(crbug.com/764578): -loadModel should not be called from
    // initializer. A possible fix is to move this call to -viewDidLoad.
    [self loadModel];
  }
  return self;
}

- (void)loadModel {
  [super loadModel];
  CollectionViewModel* model = self.collectionViewModel;

  [model addSectionWithIdentifier:SectionIdentifierSwitch];
  [model addItem:[self switchItem]
      toSectionWithIdentifier:SectionIdentifierSwitch];

  [model addSectionWithIdentifier:SectionIdentifierFooter];
  [model addItem:[self footerItem]
      toSectionWithIdentifier:SectionIdentifierFooter];
}

- (CollectionViewItem*)switchItem {
  CollectionViewSwitchItem* item =
      [[CollectionViewSwitchItem alloc] initWithType:ItemTypeSwitch];
  item.text = l10n_util::GetNSString(IDS_IOS_OPTIONS_DO_NOT_TRACK_MOBILE);
  item.on = _doNotTrackEnabled.GetValue();
  return item;
}

- (CollectionViewItem*)footerItem {
  NSString* footerText = l10n_util::GetNSString(
      IDS_IOS_OPTIONS_ENABLE_DO_NOT_TRACK_BUBBLE_TEXT_MOBILE);
  GURL learnMoreURL = google_util::AppendGoogleLocaleParam(
      GURL(kDoNotTrackLearnMoreURL),
      GetApplicationContext()->GetApplicationLocale());

  CollectionViewFooterItem* item =
      [[CollectionViewFooterItem alloc] initWithType:ItemTypeFooter];
  item.text = footerText;
  item.linkURL = learnMoreURL;
  item.linkDelegate = self;
  return item;
}

#pragma mark - Actions

- (void)switchToggled:(id)sender {
  NSIndexPath* switchPath =
      [self.collectionViewModel indexPathForItemType:ItemTypeSwitch
                                   sectionIdentifier:SectionIdentifierSwitch];

  CollectionViewSwitchItem* switchItem =
      base::mac::ObjCCastStrict<CollectionViewSwitchItem>(
          [self.collectionViewModel itemAtIndexPath:switchPath]);
  CollectionViewSwitchCell* switchCell =
      base::mac::ObjCCastStrict<CollectionViewSwitchCell>(
          [self.collectionView cellForItemAtIndexPath:switchPath]);

  // Update the model and the preference with the current value of the switch.
  DCHECK_EQ(switchCell.switchView, sender);
  BOOL isOn = switchCell.switchView.isOn;
  switchItem.on = isOn;
  _doNotTrackEnabled.SetValue(isOn);
}

#pragma mark - UICollectionViewDelegate

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  UICollectionViewCell* cell =
      [super collectionView:collectionView cellForItemAtIndexPath:indexPath];
  NSInteger itemType =
      [self.collectionViewModel itemTypeForIndexPath:indexPath];

  if (itemType == ItemTypeSwitch) {
    // Have the switch send a message on UIControlEventValueChanged.
    CollectionViewSwitchCell* switchCell =
        base::mac::ObjCCastStrict<CollectionViewSwitchCell>(cell);
    [switchCell.switchView addTarget:self
                              action:@selector(switchToggled:)
                    forControlEvents:UIControlEventValueChanged];
  }

  return cell;
}

#pragma mark - MDCCollectionViewStylingDelegate

- (CGFloat)collectionView:(UICollectionView*)collectionView
    cellHeightAtIndexPath:(NSIndexPath*)indexPath {
  CollectionViewItem* item =
      [self.collectionViewModel itemAtIndexPath:indexPath];
  switch (item.type) {
    case ItemTypeFooter:
      return [MDCCollectionViewCell
          cr_preferredHeightForWidth:CGRectGetWidth(collectionView.bounds)
                             forItem:item];
    default:
      return MDCCellDefaultOneLineHeight;
  }
}

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

- (BOOL)collectionView:(UICollectionView*)collectionView
    hidesInkViewAtIndexPath:(NSIndexPath*)indexPath {
  NSInteger type = [self.collectionViewModel itemTypeForIndexPath:indexPath];
  switch (type) {
    case ItemTypeFooter:
    case ItemTypeSwitch:
      return YES;
    default:
      return NO;
  }
}

@end
