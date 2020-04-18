// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COLLECTION_VIEW_ITEM_H_
#define IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COLLECTION_VIEW_ITEM_H_

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"

#import "ios/chrome/browser/ui/reading_list/reading_list_collection_view_cell.h"

class GURL;
@class FaviconAttributes;
@protocol ReadingListCollectionViewItemAccessibilityDelegate;

// Collection view item for representing a ReadingListEntry.
@interface ReadingListCollectionViewItem : CollectionViewItem

// The title to display.
@property(nonatomic, copy) NSString* title;
// The subtitle to display. This is often the |url|'s origin.
@property(nonatomic, copy) NSString* subtitle;
// The URL of the Reading List entry.
@property(nonatomic, readonly) const GURL& url;
// The URL of the page presenting the favicon to display.
@property(nonatomic, assign) GURL faviconPageURL;
// Status of the offline version.
@property(nonatomic, assign) ReadingListUIDistillationStatus distillationState;
// Size of the distilled files.
@property(nonatomic, assign) int64_t distillationSize;
// Timestamp of the distillation in microseconds since Jan 1st 1970.
@property(nonatomic, assign) int64_t distillationDate;
// Delegate for the accessibility actions.
@property(nonatomic, weak)
    id<ReadingListCollectionViewItemAccessibilityDelegate>
        accessibilityDelegate;
// Attributes for favicon.
@property(nonatomic, strong) FaviconAttributes* attributes;

// Designated initializer. The |provider| will be used for loading favicon
// attributes. The |delegate| is used to inform about changes to this item. The
// |url| is displayed as a subtitle. The |state| is used to display visual
// indicator of the distillation status.
- (instancetype)initWithType:(NSInteger)type
                         url:(const GURL&)url
           distillationState:(ReadingListUIDistillationStatus)state
    NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithType:(NSInteger)type NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_READING_LIST_READING_LIST_COLLECTION_VIEW_ITEM_H_
