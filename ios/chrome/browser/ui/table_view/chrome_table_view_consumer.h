// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TABLE_VIEW_CHROME_TABLE_VIEW_CONSUMER_H_
#define IOS_CHROME_BROWSER_UI_TABLE_VIEW_CHROME_TABLE_VIEW_CONSUMER_H_

#import <Foundation/Foundation.h>

// ChromeTableViewConsumer declares a basic set of methods that allow table view
// mediators to update their UI. Individual features can extend this protocol to
// add feature-specific methods.
@protocol ChromeTableViewConsumer<NSObject>

// Reconfigures the cells corresponding to the given |items| by calling
// |configureCell:| on each cell.
- (void)reconfigureCellsForItems:(NSArray*)items;

@end

#endif  // IOS_CHROME_BROWSER_UI_TABLE_VIEW_CHROME_TABLE_VIEW_CONSUMER_H_
