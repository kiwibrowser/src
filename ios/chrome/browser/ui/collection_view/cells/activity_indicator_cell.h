// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COLLECTION_VIEW_CELLS_ACTIVITY_INDICATOR_CELL_H_
#define IOS_CHROME_BROWSER_UI_COLLECTION_VIEW_CELLS_ACTIVITY_INDICATOR_CELL_H_

#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"

extern const CGFloat kIndicatorSize;

@class MDCActivityIndicator;

// MDCCollectionViewCell that displays an activity indicator.
@interface ActivityIndicatorCell : MDCCollectionViewCell

@property(nonatomic, strong) MDCActivityIndicator* activityIndicator;

@end
#endif  // IOS_CHROME_BROWSER_UI_COLLECTION_VIEW_CELLS_ACTIVITY_INDICATOR_CELL_H_
