// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history_popup/tab_history_view_controller.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "ios/chrome/browser/ui/commands/history_popup_commands.h"
#import "ios/chrome/browser/ui/history_popup/tab_history_cell.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_constants.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/third_party/material_components_ios/src/components/Ink/src/MaterialInk.h"
#include "ios/web/public/favicon_status.h"
#include "ios/web/public/navigation_item.h"
#include "ui/gfx/image/image.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Visible percentage of the last visible row on the Tools menu if the
// Tools menu is scrollable.
const CGFloat kLastRowVisiblePercentage = 0.6;
// Reuse identifier for cells.
NSString* const kCellIdentifier = @"TabHistoryCell";
NSString* const kFooterIdentifier = @"Footer";
NSString* const kHeaderIdentifier = @"Header";
// Height of rows.
const CGFloat kCellHeight = 48.0;
// Fraction height for partially visible row.
const CGFloat kCellHeightLastRow = kCellHeight * kLastRowVisiblePercentage;
// Width and leading for the header view.
const CGFloat kHeaderLeadingInset = 0;
const CGFloat kHeaderWidth = 48;
// Leading and trailing insets for cell items.
const CGFloat kCellLeadingInset = kHeaderLeadingInset + kHeaderWidth;
const CGFloat kCellTrailingInset = 16;

typedef std::vector<CGFloat> CGFloatVector;
typedef std::vector<CGFloatVector> ItemOffsetVector;

NS_INLINE CGFloat OffsetForPath(const ItemOffsetVector& offsets,
                                NSInteger section,
                                NSInteger item) {
  DCHECK(section < (NSInteger)offsets.size());
  DCHECK(item < (NSInteger)offsets.at(section).size());

  return offsets.at(section).at(item);
}

NS_INLINE CGFloat OffsetForPath(const ItemOffsetVector& offsets,
                                NSIndexPath* path) {
  return OffsetForPath(offsets, [path section], [path item]);
}

NS_INLINE CGFloat OffsetForSection(const ItemOffsetVector& offsets,
                                   NSIndexPath* path) {
  DCHECK([path section] < (NSInteger)offsets.size());
  DCHECK(offsets.at([path section]).size());

  return offsets.at([path section]).at(0);
}

// Height for the footer view.
NS_INLINE CGFloat FooterHeight() {
  return 1.0 / [[UIScreen mainScreen] scale];
}

// Returns a vector of of NavigationItemLists where the NavigationItems in
// |items| are separated by host.
NS_INLINE std::vector<web::NavigationItemList> PartitionItemsByHost(
    const web::NavigationItemList& items) {
  std::vector<web::NavigationItemList> partitionedItems;
  // Used to store the previous host when partitioning NavigationItems.
  std::string previousHost;
  // The NavigationItemList containing NavigationItems with the same host.
  web::NavigationItemList itemsWithSameHostname;
  // Separate the items in |items| by host.
  for (web::NavigationItem* item : items) {
    std::string currentHost = item->GetURL().host();
    if (previousHost.empty())
      previousHost = currentHost;
    // TODO: This should use some sort of Top Level Domain matching instead of
    // explicit host match so that images.googe.com matches shopping.google.com.
    if (previousHost == currentHost) {
      itemsWithSameHostname.push_back(item);
    } else {
      partitionedItems.push_back(itemsWithSameHostname);
      itemsWithSameHostname = web::NavigationItemList(1, item);
      previousHost = currentHost;
    }
  }
  // Add the last list contiaining the same host.
  if (!itemsWithSameHostname.empty())
    partitionedItems.push_back(itemsWithSameHostname);
  return partitionedItems;
}

}  // namespace

@interface TabHistoryViewControllerLayout : UICollectionViewLayout
@end

@implementation TabHistoryViewControllerLayout {
  ItemOffsetVector _itemYOffsets;
  CGFloat _containerCalculatedHeight;
  CGFloat _containerWidth;
  CGFloat _cellItemWidth;
  CGFloat _footerWidth;
}

- (void)prepareLayout {
  [super prepareLayout];

  UICollectionView* collectionView = [self collectionView];
  CGFloat yOffset = 0;

  NSInteger numberOfSections = [collectionView numberOfSections];
  _itemYOffsets.reserve(numberOfSections);

  for (NSInteger section = 0; section < numberOfSections; ++section) {
    NSInteger numberOfItems = [collectionView numberOfItemsInSection:section];

    CGFloatVector dummy;
    _itemYOffsets.push_back(dummy);

    CGFloatVector& sectionYOffsets = _itemYOffsets.at(section);
    sectionYOffsets.reserve(numberOfItems);

    for (NSInteger item = 0; item < numberOfItems; ++item) {
      sectionYOffsets.push_back(yOffset);
      yOffset += kCellHeight;
    }

    // The last section should not have a footer.
    if (numberOfItems && (section + 1) < numberOfSections) {
      yOffset += FooterHeight();
    }
  }

  CGRect containerBounds = [collectionView bounds];
  _containerWidth = CGRectGetWidth(containerBounds);
  _cellItemWidth = _containerWidth - kCellLeadingInset - kCellTrailingInset;
  _footerWidth = _containerWidth - kCellLeadingInset;
  _containerCalculatedHeight = yOffset - kCellHeight / 2.0;
}

- (void)invalidateLayout {
  [super invalidateLayout];
  _itemYOffsets.clear();
  _containerCalculatedHeight = 0;
  _cellItemWidth = 0;
  _footerWidth = 0;
}

- (CGSize)collectionViewContentSize {
  return CGSizeMake(_containerWidth, _containerCalculatedHeight);
}

- (NSArray*)layoutAttributesForElementsInRect:(CGRect)rect {
  UICollectionView* collectionView = [self collectionView];
  NSMutableArray* array = [NSMutableArray array];

  NSInteger numberOfSections = [collectionView numberOfSections];
  for (NSInteger section = 0; section < numberOfSections; ++section) {
    NSInteger numberOfItems = [collectionView numberOfItemsInSection:section];
    if (numberOfItems) {
      NSIndexPath* path =
          [NSIndexPath indexPathForItem:numberOfItems - 1 inSection:section];
      [array addObject:[self layoutAttributesForSupplementaryViewOfKind:
                                 UICollectionElementKindSectionHeader
                                                            atIndexPath:path]];
    }

    for (NSInteger item = 0; item < numberOfItems; ++item) {
      NSIndexPath* path = [NSIndexPath indexPathForItem:item inSection:section];
      [array addObject:[self layoutAttributesForItemAtIndexPath:path]];
    }

    // The last section should not have a footer.
    if (numberOfItems && (section + 1) < numberOfSections) {
      NSIndexPath* path =
          [NSIndexPath indexPathForItem:numberOfItems - 1 inSection:section];
      [array addObject:[self layoutAttributesForSupplementaryViewOfKind:
                                 UICollectionElementKindSectionFooter
                                                            atIndexPath:path]];
    }
  }

  return array;
}

- (UICollectionViewLayoutAttributes*)layoutAttributesForItemAtIndexPath:
    (NSIndexPath*)indexPath {
  CGFloat yOffset = OffsetForPath(_itemYOffsets, indexPath);

  UICollectionViewLayoutAttributes* attributes =
      [UICollectionViewLayoutAttributes
          layoutAttributesForCellWithIndexPath:indexPath];
  LayoutRect cellLayout = LayoutRectMake(kCellLeadingInset, _containerWidth,
                                         yOffset, _cellItemWidth, kCellHeight);
  [attributes setFrame:LayoutRectGetRect(cellLayout)];
  [attributes setZIndex:1];

  return attributes;
}

- (UICollectionViewLayoutAttributes*)
layoutAttributesForSupplementaryViewOfKind:(NSString*)kind
                               atIndexPath:(NSIndexPath*)indexPath {
  CGFloat yOffset = OffsetForPath(_itemYOffsets, indexPath);

  UICollectionViewLayoutAttributes* attributes =
      [UICollectionViewLayoutAttributes
          layoutAttributesForSupplementaryViewOfKind:kind
                                       withIndexPath:indexPath];

  if ([kind isEqualToString:UICollectionElementKindSectionHeader]) {
    // The height is the yOffset of the first section minus the yOffset of the
    // last item. Additionally, the height of a cell needs to be added back in
    // since the _itemYOffsets stores Mid Y.
    CGFloat headerYOffset = OffsetForSection(_itemYOffsets, indexPath);
    CGFloat height = yOffset - headerYOffset + kCellHeight;

    if ([indexPath section])
      headerYOffset += FooterHeight();

    LayoutRect cellLayout = LayoutRectMake(kHeaderLeadingInset, _containerWidth,
                                           headerYOffset, kHeaderWidth, height);
    [attributes setFrame:LayoutRectGetRect(cellLayout)];
    [attributes setZIndex:0];
  } else if ([kind isEqualToString:UICollectionElementKindSectionFooter]) {
    LayoutRect cellLayout =
        LayoutRectMake(kCellLeadingInset, _containerWidth, yOffset,
                       _footerWidth, FooterHeight());
    [attributes setFrame:LayoutRectGetRect(cellLayout)];
    [attributes setZIndex:0];
  }

  return attributes;
}

@end

@interface TabHistoryViewController ()<MDCInkTouchControllerDelegate> {
  MDCInkTouchController* _inkTouchController;
  // A vector of NavigationItemLists where the NavigationItems are separated
  // by hostname.
  std::vector<web::NavigationItemList> _partitionedItems;
}

// Returns the NavigationItem corresponding with |indexPath|.
- (const web::NavigationItem*)itemAtIndexPath:(NSIndexPath*)indexPath;

// Removes all NavigationItem pointers from this class.  Tapping a cell that
// triggers a navigation may delete NavigationItems, so NavigationItem
// references should be reset to avoid use-after-free errors.
- (void)clearNavigationItems;

// The dispatcher used by this ViewController.
@property(nonatomic, readonly, weak) id<TabHistoryPopupCommands> dispatcher;

@end

@implementation TabHistoryViewController

@synthesize dispatcher = _dispatcher;

- (instancetype)initWithItems:(const web::NavigationItemList&)items
                   dispatcher:(id<TabHistoryPopupCommands>)dispatcher {
  TabHistoryViewControllerLayout* layout =
      [[TabHistoryViewControllerLayout alloc] init];
  if ((self = [super initWithCollectionViewLayout:layout])) {
    // Populate |_partitionedItems|.
    _partitionedItems = PartitionItemsByHost(items);

    // Set up the dispatcher.
    _dispatcher = dispatcher;

    // Set up the UICollectionView.
    UICollectionView* collectionView = [self collectionView];
    collectionView.accessibilityIdentifier = kPopupMenuNavigationTableViewId;
    collectionView.backgroundColor = [UIColor whiteColor];
    [collectionView registerClass:[TabHistoryCell class]
        forCellWithReuseIdentifier:kCellIdentifier];
    [collectionView registerClass:[TabHistorySectionHeader class]
        forSupplementaryViewOfKind:UICollectionElementKindSectionHeader
               withReuseIdentifier:kHeaderIdentifier];
    [collectionView registerClass:[TabHistorySectionFooter class]
        forSupplementaryViewOfKind:UICollectionElementKindSectionFooter
               withReuseIdentifier:kFooterIdentifier];

    // Set up the ink controller.
    _inkTouchController =
        [[MDCInkTouchController alloc] initWithView:collectionView];
    [_inkTouchController setDelegate:self];
    [_inkTouchController addInkView];
  }
  return self;
}

#pragma mark Public Methods

- (CGFloat)optimalHeight:(CGFloat)suggestedHeight {
  DCHECK(suggestedHeight >= kCellHeight);
  CGFloat optimalHeight = 0;

  for (web::NavigationItemList& itemsWithSameHost : _partitionedItems) {
    for (size_t count = 0; count < itemsWithSameHost.size(); ++count) {
      CGFloat proposedHeight = optimalHeight + kCellHeight;

      if (proposedHeight > suggestedHeight) {
        CGFloat difference = proposedHeight - suggestedHeight;
        if (difference > kCellHeightLastRow) {
          return optimalHeight + kCellHeightLastRow;
        } else {
          return optimalHeight - kCellHeight + kCellHeightLastRow;
        }
      }

      optimalHeight = proposedHeight;
    }

    optimalHeight += FooterHeight();
  }

  // If this point is reached, it means the entire content fits and this last
  // section should not include the footer.
  optimalHeight -= FooterHeight();

  return optimalHeight;
}

#pragma mark UICollectionViewDelegate

- (void)collectionView:(UICollectionView*)collectionView
    didSelectItemAtIndexPath:(NSIndexPath*)indexPath {
  TabHistoryCell* cell = base::mac::ObjCCastStrict<TabHistoryCell>(
      [collectionView cellForItemAtIndexPath:indexPath]);
  [self.dispatcher navigateToHistoryItem:cell.item];
  [self clearNavigationItems];
}

#pragma mark UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView*)collectionView
     numberOfItemsInSection:(NSInteger)section {
  size_t sectionIdx = static_cast<size_t>(section);
  DCHECK_LT(sectionIdx, _partitionedItems.size());
  return _partitionedItems[sectionIdx].size();
}

- (UICollectionViewCell*)collectionView:(UICollectionView*)collectionView
                 cellForItemAtIndexPath:(NSIndexPath*)indexPath {
  TabHistoryCell* cell =
      [collectionView dequeueReusableCellWithReuseIdentifier:kCellIdentifier
                                                forIndexPath:indexPath];
  cell.item = [self itemAtIndexPath:indexPath];
  return cell;
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView*)view {
  return _partitionedItems.size();
}

- (UICollectionReusableView*)collectionView:(UICollectionView*)view
          viewForSupplementaryElementOfKind:(NSString*)kind
                                atIndexPath:(NSIndexPath*)indexPath {
  // Return a footer cell if requested.
  if ([kind isEqualToString:UICollectionElementKindSectionFooter]) {
    return [view dequeueReusableSupplementaryViewOfKind:kind
                                    withReuseIdentifier:kFooterIdentifier
                                           forIndexPath:indexPath];
  }
  DCHECK([kind isEqualToString:UICollectionElementKindSectionHeader]);

  // Dequeue a header cell and populate its favicon image.
  TabHistorySectionHeader* header =
      [view dequeueReusableSupplementaryViewOfKind:kind
                               withReuseIdentifier:kHeaderIdentifier
                                      forIndexPath:indexPath];
  UIImage* iconImage = nil;
  const gfx::Image& image =
      [self itemAtIndexPath:indexPath]->GetFavicon().image;
  if (!image.IsEmpty())
    iconImage = image.ToUIImage();
  else
    iconImage = [UIImage imageNamed:@"default_favicon"];
  [[header iconView] setImage:iconImage];

  return header;
}

#pragma mark MDCInkTouchControllerDelegate

- (BOOL)inkTouchController:(MDCInkTouchController*)inkTouchController
    shouldProcessInkTouchesAtTouchLocation:(CGPoint)location {
  NSIndexPath* indexPath =
      [self.collectionView indexPathForItemAtPoint:location];
  TabHistoryCell* cell = base::mac::ObjCCastStrict<TabHistoryCell>(
      [self.collectionView cellForItemAtIndexPath:indexPath]);
  if (!cell) {
    return NO;
  }

  // Increase the size of the ink view to cover the collection view from edge
  // to edge.
  CGRect inkViewFrame = [cell frame];
  inkViewFrame.origin.x = 0;
  inkViewFrame.size.width = CGRectGetWidth([self.collectionView bounds]);
  [[inkTouchController defaultInkView] setFrame:inkViewFrame];
  return YES;
}

#pragma mark -

- (const web::NavigationItem*)itemAtIndexPath:(NSIndexPath*)indexPath {
  size_t section = static_cast<size_t>([indexPath section]);
  size_t item = static_cast<size_t>([indexPath item]);
  DCHECK_LT(section, _partitionedItems.size());
  DCHECK_LT(item, _partitionedItems[section].size());
  return _partitionedItems[section][item];
}

- (void)clearNavigationItems {
  _partitionedItems.clear();
  for (UICollectionViewCell* cell in self.collectionView.visibleCells) {
    TabHistoryCell* historyCell = base::mac::ObjCCast<TabHistoryCell>(cell);
    historyCell.item = nullptr;
  }
}

@end
