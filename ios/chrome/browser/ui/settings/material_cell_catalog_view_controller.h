// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_MATERIAL_CELL_CATALOG_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_MATERIAL_CELL_CATALOG_VIEW_CONTROLLER_H_

#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"

@interface MaterialCellCatalogViewController
    : SettingsRootCollectionViewController

- (instancetype)init NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_MATERIAL_CELL_CATALOG_VIEW_CONTROLLER_H_
