// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_MEDIATOR_H_
#define IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_MEDIATOR_H_

#import <Foundation/Foundation.h>

#import "ios/chrome/browser/ui/tab_grid/grid/grid_commands.h"
#import "ios/chrome/browser/ui/tab_grid/grid/grid_image_data_source.h"

@protocol GridConsumer;
@class TabModel;

// Mediates between model layer and tab grid UI layer.
@interface TabGridMediator : NSObject<GridCommands, GridImageDataSource>

// The source tab model.
@property(nonatomic, weak) TabModel* tabModel;

// Initializer with |consumer| as the receiver of model layer updates.
- (instancetype)initWithConsumer:(id<GridConsumer>)consumer
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
@end

#endif  // IOS_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_MEDIATOR_H_
