// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABLE_VIEW_TABLE_VIEW_MODEL_H_
#define IOS_CHROME_BROWSER_UI_TABLE_VIEW_TABLE_VIEW_MODEL_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/list_model/list_model.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_header_footer_item.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_item.h"

// Key for saving collapsed state in the UserDefaults.
extern NSString* const kTableViewModelCollapsedKey;

// TableViewModel acts as a model class for table view controllers.
@interface TableViewModel<__covariant ObjectType : TableViewItem*> :
    ListModel<ObjectType, TableViewHeaderFooterItem*>

// Sets an existing |sectionIdentifier| |collapsedKey| to be used when
// collapsing or expanding a section. |collapsedKey| is a unique identifier for
// each section that will be used for persisting information about the collapsed
// state of a section. A |collapsedKey| its only needed when
// collapsing/expanding sections. You can't collapse/expand any sections without
// a |collapsedKey|.
- (void)setSectionIdentifier:(NSInteger)sectionIdentifier
                collapsedKey:(NSString*)collapsedKey;
// Sets the state of an existing |sectionIdentifier| to |collapsed|. A
// collapsedKey has to be previously set or this method will DCHECK().
- (void)setSection:(NSInteger)sectionIdentifier collapsed:(BOOL)collapsed;
// Returns YES if |sectionIdentifier| is collapsed. If not collapsedKey has been
// set it will also return NO.
- (BOOL)sectionIsCollapsed:(NSInteger)sectionIdentifier;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABLE_VIEW_TABLE_VIEW_MODEL_H_
