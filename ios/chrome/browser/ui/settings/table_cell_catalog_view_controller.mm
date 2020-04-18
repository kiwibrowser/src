// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/table_cell_catalog_view_controller.h"

#import "ios/chrome/browser/ui/table_view/cells/table_view_accessory_item.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_text_button_item.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_text_header_footer_item.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_text_item.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_url_item.h"
#import "ios/chrome/browser/ui/table_view/table_view_model.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierText = kSectionIdentifierEnumZero,
  SectionIdentifierURL,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeText = kItemTypeEnumZero,
  ItemTypeTextHeader,
  ItemTypeTextFooter,
  ItemTypeTextButton,
  ItemTypeURLNoMetadata,
  ItemTypeTextAccessoryImage,
  ItemTypeTextAccessoryNoImage,
  ItemTypeURLWithTimestamp,
  ItemTypeURLWithSize,
};
}

@implementation TableCellCatalogViewController

- (instancetype)init {
  if ((self = [super
           initWithTableViewStyle:UITableViewStyleGrouped
                      appBarStyle:ChromeTableViewControllerStyleWithAppBar])) {
  }
  return self;
}

- (void)viewDidLoad {
  [super viewDidLoad];
  self.title = @"Table Cell Catalog";
  self.tableView.rowHeight = UITableViewAutomaticDimension;
  self.tableView.estimatedRowHeight = 56.0;
  self.tableView.estimatedSectionHeaderHeight = 56.0;
  self.tableView.estimatedSectionFooterHeight = 56.0;

  [self loadModel];
}

- (void)loadModel {
  [super loadModel];

  TableViewModel* model = self.tableViewModel;
  [model addSectionWithIdentifier:SectionIdentifierText];
  [model addSectionWithIdentifier:SectionIdentifierURL];

  // SectionIdentifierText.
  TableViewTextHeaderFooterItem* textHeaderFooterItem =
      [[TableViewTextHeaderFooterItem alloc] initWithType:ItemTypeTextHeader];
  textHeaderFooterItem.text = @"Simple Text Header";
  [model setHeader:textHeaderFooterItem
      forSectionWithIdentifier:SectionIdentifierText];

  TableViewTextItem* textItem =
      [[TableViewTextItem alloc] initWithType:ItemTypeText];
  textItem.text = @"Simple Text Cell";
  textItem.textAlignment = NSTextAlignmentCenter;
  textItem.textColor = TextItemColorBlack;
  [model addItem:textItem toSectionWithIdentifier:SectionIdentifierText];

  TableViewAccessoryItem* textAccessoryItem =
      [[TableViewAccessoryItem alloc] initWithType:ItemTypeTextAccessoryImage];
  textAccessoryItem.title = @"Text Accessory with History Image";
  textAccessoryItem.image = [UIImage imageNamed:@"show_history"];
  [model addItem:textAccessoryItem
      toSectionWithIdentifier:SectionIdentifierText];

  textAccessoryItem = [[TableViewAccessoryItem alloc]
      initWithType:ItemTypeTextAccessoryNoImage];
  textAccessoryItem.title = @"Text Accessory No Image";
  [model addItem:textAccessoryItem
      toSectionWithIdentifier:SectionIdentifierText];

  TableViewTextItem* textItemDefault =
      [[TableViewTextItem alloc] initWithType:ItemTypeText];
  textItemDefault.text = @"Simple Text Cell with Defaults";
  [model addItem:textItemDefault toSectionWithIdentifier:SectionIdentifierText];

  textHeaderFooterItem =
      [[TableViewTextHeaderFooterItem alloc] initWithType:ItemTypeTextFooter];
  textHeaderFooterItem.text = @"Simple Text Footer";
  [model setFooter:textHeaderFooterItem
      forSectionWithIdentifier:SectionIdentifierText];

  TableViewTextButtonItem* textActionButtonItem =
      [[TableViewTextButtonItem alloc] initWithType:ItemTypeTextButton];
  textActionButtonItem.text = @"Hello, you should do something.";
  textActionButtonItem.buttonText = @"Do something";
  [model setFooter:textActionButtonItem
      forSectionWithIdentifier:SectionIdentifierText];

  // SectionIdentifierURL.
  TableViewURLItem* item =
      [[TableViewURLItem alloc] initWithType:ItemTypeURLNoMetadata];
  item.title = @"Google Design";
  item.URL = GURL("https://design.google.com");
  [model addItem:item toSectionWithIdentifier:SectionIdentifierURL];

  item = [[TableViewURLItem alloc] initWithType:ItemTypeURLNoMetadata];
  item.URL = GURL("https://notitle.google.com");
  [model addItem:item toSectionWithIdentifier:SectionIdentifierURL];

  item = [[TableViewURLItem alloc] initWithType:ItemTypeURLWithTimestamp];
  item.title = @"Google";
  item.URL = GURL("https://www.google.com");
  item.metadata = @"3:42 PM";
  [model addItem:item toSectionWithIdentifier:SectionIdentifierURL];

  item = [[TableViewURLItem alloc] initWithType:ItemTypeURLWithSize];
  item.title = @"World Series 2017: Houston Astros Defeat Someone Else";
  item.URL = GURL("https://m.bbc.com");
  item.metadata = @"176 KB";
  [model addItem:item toSectionWithIdentifier:SectionIdentifierURL];
}

@end
