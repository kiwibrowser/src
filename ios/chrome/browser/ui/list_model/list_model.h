// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_LIST_MODEL_LIST_MODEL_H_
#define IOS_CHROME_BROWSER_UI_LIST_MODEL_LIST_MODEL_H_

#import <UIKit/UIKit.h>

@class ListItem;

// Use these as the starting value for section identifier and item type enums.
// These are provided to help not mix between indexPath's section/item and the
// model section identifier / item type.
//
// For example:
// typedef NS_ENUM(NSInteger, SectionIdentifier) {
//   SectionIdentifierFoo = kSectionIdentifierEnumZero,
//   SectionIdentifierBar,
// };
//
// typedef NS_ENUM(NSInteger, ItemType) {
//   ItemTypeBaz = kItemTypeEnumZero,
//   ItemTypeQux,
// };
//
// These values are chosen to try to prevent overlapping.
const NSInteger kSectionIdentifierEnumZero = 10;
const NSInteger kItemTypeEnumZero = 100;

// ListModel acts as a model class for collectionview and tableview controllers.
// It provides methods to map from index paths (aka section and item) to model
// coordinates. These are useful when the contents of a list are dynamic (for
// example, based on experimental flags), so that there is not a static mapping
// between index paths and section items.  Model coordinates have 3 dimensions:
// section identifier, item type and index in item type.
//
// Disclaimer: ListModel doesn't support a batch update logic. All changes are
// immediately processed (contrary to the reload/delete/add order of
// |performBatchUpdates:completion:|).  The __covariant modifier allows an
// instance of ListModel<A,B> to be assigned to an instance of ListModel<X,Y>
// iff A:X and B:Y (see unit test for an example).
@interface ListModel<__covariant ObjectType : ListItem*,
                     __covariant SupplementalType : ListItem*> : NSObject

#pragma mark Modification methods

// Adds a new section tagged with the given identifier. This method must not be
// called multiple times with the same identifier.
- (void)addSectionWithIdentifier:(NSInteger)sectionIdentifier;

// Inserts a new section tagged with the given identifier at the given
// index. This method must not be called multiple times with the same
// identifier.
- (void)insertSectionWithIdentifier:(NSInteger)sectionIdentifier
                            atIndex:(NSUInteger)index;

// Adds an item to the section with the given identifier. Adding several
// times the same item is undefined behavior.
- (void)addItem:(ObjectType)item
    toSectionWithIdentifier:(NSInteger)sectionIdentifier;

// Inserts an item to the section with the given identifier at the given
// index. |index| must not be greater than the count of elements in the
// section.
- (void)insertItem:(ObjectType)item
    inSectionWithIdentifier:(NSInteger)sectionIdentifier
                    atIndex:(NSUInteger)index;

// Removes the item for |itemType| from the section for |sectionIdentifier|.
// If there are multiple entries with the same item type, this will remove
// the first occurrence, but to selectively delete an item for a given
// index, use -removeItemWithType:fromSectionWithIdentifier:atIndex:.
- (void)removeItemWithType:(NSInteger)itemType
    fromSectionWithIdentifier:(NSInteger)sectionIdentifier;

// Removes the item for |itemType| from the section for |sectionIdentifier|
// at |index|.
- (void)removeItemWithType:(NSInteger)itemType
    fromSectionWithIdentifier:(NSInteger)sectionIdentifier
                      atIndex:(NSUInteger)index;

// Removes the section for |sectionIdentifier|. If there are still items
// left in the section, they are removed.
- (void)removeSectionWithIdentifier:(NSInteger)sectionIdentifier;

// Sets the header item for the section with the given |sectionIdentifier|.
- (void)setHeader:(SupplementalType)header
    forSectionWithIdentifier:(NSInteger)sectionIdentifier;

// Sets the footer item for the section with the given |sectionIdentifier|.
- (void)setFooter:(SupplementalType)footer
    forSectionWithIdentifier:(NSInteger)sectionIdentifier;

#pragma mark Query model coordinates from index paths

// Returns the section identifier for the given section.
- (NSInteger)sectionIdentifierForSection:(NSInteger)section;

// Returns the item type for the given index path.
- (NSInteger)itemTypeForIndexPath:(NSIndexPath*)indexPath;

// Returns the index in item type for the given index path.
// It corresponds to the index of the item at index path, among all the
// items if the same type in the given section. For example, let [A, B, B,
// B] a section, where A and B are item identifiers.
// -indexInItemTypeForIndexPath:{0, 0} would return 0.
// -indexInItemTypeForIndexPath:{0, 1} would return 0.
// -indexInItemTypeForIndexPath:{0, 3} would return 2.
- (NSUInteger)indexInItemTypeForIndexPath:(NSIndexPath*)indexPath;

#pragma mark Query items from index paths

// Returns whether there is an item at the given index path.
- (BOOL)hasItemAtIndexPath:(NSIndexPath*)indexPath;

// Returns the item at the given index path.
- (ObjectType)itemAtIndexPath:(NSIndexPath*)indexPath;

// Returns the header for the given |section|.
- (SupplementalType)headerForSection:(NSInteger)section;

// Returns the footer for the given |section|.
- (SupplementalType)footerForSection:(NSInteger)section;

// Returns an array of items in the section with the given identifier.
- (NSArray<ObjectType>*)itemsInSectionWithIdentifier:
    (NSInteger)sectionIdentifier;

// Returns the header for the section with the given |sectionIdentifier|.
- (SupplementalType)headerForSectionWithIdentifier:(NSInteger)sectionIdentifier;

// Returns the footer for the section with the given |sectionIdentifier|.
- (SupplementalType)footerForSectionWithIdentifier:(NSInteger)sectionIdentifier;

#pragma mark Query index paths from model coordinates

// Returns whether there is a section at the given section identifier.
- (BOOL)hasSectionForSectionIdentifier:(NSInteger)sectionIdentifier;

// Returns the index path's section for the given section identifier.
- (NSInteger)sectionForSectionIdentifier:(NSInteger)sectionIdentifier;

// Returns whether there is an item of type |itemType| at the given
// |sectionIdentifier|.
- (BOOL)hasItemForItemType:(NSInteger)itemType
         sectionIdentifier:(NSInteger)sectionIdentifier;

// Returns the index path for |itemType| in the section for
// |sectionIdentifier|. If there are multiple entries with the same item
// type, use -indexPathForItemType:sectionIdentifier:atIndex:.
- (NSIndexPath*)indexPathForItemType:(NSInteger)itemType
                   sectionIdentifier:(NSInteger)sectionIdentifier;

// Returns whether there is an item of type |itemType| at the given
// |sectionIdentifier| and |index|.
- (BOOL)hasItemForItemType:(NSInteger)itemType
         sectionIdentifier:(NSInteger)sectionIdentifier
                   atIndex:(NSUInteger)index;

// Returns the index path for |itemType| in the section for
// |sectionIdentifier| at |index|.
- (NSIndexPath*)indexPathForItemType:(NSInteger)itemType
                   sectionIdentifier:(NSInteger)sectionIdentifier
                             atIndex:(NSUInteger)index;

#pragma mark Query index paths from items

// Returns whether |item| exists in section for |sectionIdentifier|.
- (BOOL)hasItem:(ObjectType)item
    inSectionWithIdentifier:(NSInteger)sectionIdentifier;

// Returns whether |item| exists.
- (BOOL)hasItem:(ObjectType)item;

// Returns the index path corresponding to the given |item|.
- (NSIndexPath*)indexPathForItem:(ObjectType)item;

#pragma mark Data sourcing

// Returns the number of sections.
- (NSInteger)numberOfSections;

// Returns the number of items in the given section.
- (NSInteger)numberOfItemsInSection:(NSInteger)section;

@end

#endif  // IOS_CHROME_BROWSER_UI_LIST_MODEL_LIST_MODEL_H_
