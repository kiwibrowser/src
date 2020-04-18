// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/table_view/table_view_model.h"

#import "ios/chrome/browser/ui/table_view/cells/table_view_header_footer_item.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_item.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

typedef NS_ENUM(NSInteger, SectionIdentifier) {
  SectionIdentifierFoo = kSectionIdentifierEnumZero,
  SectionIdentifierBar,
};

typedef NS_ENUM(NSInteger, ItemType) {
  ItemTypeFooBar = kItemTypeEnumZero,
};

class TableViewModelTest : public PlatformTest {
 protected:
  TableViewModelTest() {
    // Need to clean up NSUserDefaults before and after each test.
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:nil forKey:kTableViewModelCollapsedKey];

    model = [[TableViewModel alloc] init];

    [model addSectionWithIdentifier:SectionIdentifierFoo];
    [model setSectionIdentifier:SectionIdentifierFoo collapsedKey:@"FooKey"];
    TableViewHeaderFooterItem* header =
        [[TableViewHeaderFooterItem alloc] initWithType:ItemTypeFooBar];
    TableViewItem* item = [[TableViewItem alloc] initWithType:ItemTypeFooBar];
    [model setHeader:header forSectionWithIdentifier:SectionIdentifierFoo];
    [model addItem:item toSectionWithIdentifier:SectionIdentifierFoo];

    [model addSectionWithIdentifier:SectionIdentifierBar];
    [model setSectionIdentifier:SectionIdentifierBar collapsedKey:@"BarKey"];
    header = [[TableViewHeaderFooterItem alloc] initWithType:ItemTypeFooBar];
    item = [[TableViewItem alloc] initWithType:ItemTypeFooBar];
    [model setHeader:header forSectionWithIdentifier:SectionIdentifierBar];
    [model addItem:item toSectionWithIdentifier:SectionIdentifierBar];
  }

  ~TableViewModelTest() override {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:nil forKey:kTableViewModelCollapsedKey];
  }

  TableViewModel* model;
};

// Tests the default collapsed value is NO.
TEST_F(TableViewModelTest, DefaultCollapsedSectionValue) {
  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierBar]);
}

// Collapses all sections.
TEST_F(TableViewModelTest, SetAllCollapsed) {
  [model setSection:SectionIdentifierFoo collapsed:YES];
  [model setSection:SectionIdentifierBar collapsed:YES];

  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierBar]);

  [model setSection:SectionIdentifierFoo collapsed:NO];
  [model setSection:SectionIdentifierBar collapsed:NO];

  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierBar]);
}

// Collapses just one section at the time.
TEST_F(TableViewModelTest, SetSomeCollapsed) {
  [model setSection:SectionIdentifierFoo collapsed:NO];
  [model setSection:SectionIdentifierBar collapsed:YES];

  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierBar]);

  [model setSection:SectionIdentifierFoo collapsed:YES];
  [model setSection:SectionIdentifierBar collapsed:NO];

  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierBar]);
}

// Removes a collapsed section.
TEST_F(TableViewModelTest, RemoveCollapsedSection) {
  [model setSection:SectionIdentifierFoo collapsed:NO];
  [model setSection:SectionIdentifierBar collapsed:YES];

  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierBar]);

  EXPECT_EQ(2, [model numberOfSections]);
  [model removeSectionWithIdentifier:SectionIdentifierBar];
  EXPECT_EQ(1, [model numberOfSections]);

  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
}

// Removes a collapsed section, then re-adds it, it should still be collapsed.
TEST_F(TableViewModelTest, RemoveReaddCollapsedSection) {
  [model setSection:SectionIdentifierFoo collapsed:NO];
  [model setSection:SectionIdentifierBar collapsed:YES];

  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierBar]);

  EXPECT_EQ(2, [model numberOfSections]);
  [model removeSectionWithIdentifier:SectionIdentifierBar];
  EXPECT_EQ(1, [model numberOfSections]);

  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);

  [model addSectionWithIdentifier:SectionIdentifierBar];
  // Use the same Key as the previously removed section.
  [model setSectionIdentifier:SectionIdentifierBar collapsedKey:@"BarKey"];
  TableViewHeaderFooterItem* header =
      [[TableViewHeaderFooterItem alloc] initWithType:ItemTypeFooBar];
  TableViewItem* item = [[TableViewItem alloc] initWithType:ItemTypeFooBar];
  [model setHeader:header forSectionWithIdentifier:SectionIdentifierBar];
  [model addItem:item toSectionWithIdentifier:SectionIdentifierBar];

  EXPECT_EQ(2, [model numberOfSections]);
  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierBar]);
  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
}

// Test Collapsed persistance.
TEST_F(TableViewModelTest, PersistCollapsedSections) {
  [model setSection:SectionIdentifierFoo collapsed:NO];
  [model setSection:SectionIdentifierBar collapsed:YES];

  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierBar]);

  TableViewModel* anotherModel = [[TableViewModel alloc] init];

  [anotherModel addSectionWithIdentifier:SectionIdentifierFoo];
  [anotherModel setSectionIdentifier:SectionIdentifierFoo
                        collapsedKey:@"FooKey"];
  TableViewHeaderFooterItem* header =
      [[TableViewHeaderFooterItem alloc] initWithType:ItemTypeFooBar];
  TableViewItem* item = [[TableViewItem alloc] initWithType:ItemTypeFooBar];
  [anotherModel setHeader:header forSectionWithIdentifier:SectionIdentifierFoo];
  [anotherModel addItem:item toSectionWithIdentifier:SectionIdentifierFoo];

  [anotherModel addSectionWithIdentifier:SectionIdentifierBar];
  [anotherModel setSectionIdentifier:SectionIdentifierBar
                        collapsedKey:@"BarKey"];
  header = [[TableViewHeaderFooterItem alloc] initWithType:ItemTypeFooBar];
  item = [[TableViewItem alloc] initWithType:ItemTypeFooBar];
  [anotherModel setHeader:header forSectionWithIdentifier:SectionIdentifierBar];
  [anotherModel addItem:item toSectionWithIdentifier:SectionIdentifierBar];

  // Since the Keys are the same as the previous model it should have preserved
  // its collapsed values.
  EXPECT_FALSE([model sectionIsCollapsed:SectionIdentifierFoo]);
  EXPECT_TRUE([model sectionIsCollapsed:SectionIdentifierBar]);
}

}  // namespace
