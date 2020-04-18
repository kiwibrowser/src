// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SETTINGS_TIME_RANGE_SELECTOR_COLLECTION_VIEW_CONTROLLER_H_
#define IOS_CHROME_BROWSER_UI_SETTINGS_TIME_RANGE_SELECTOR_COLLECTION_VIEW_CONTROLLER_H_

#include "components/browsing_data/core/browsing_data_utils.h"
#import "ios/chrome/browser/ui/settings/settings_root_collection_view_controller.h"

class PrefService;
@class TimeRangeSelectorCollectionViewController;

@protocol TimeRangeSelectorCollectionViewControllerDelegate<NSObject>

// Informs the delegate that |timePeriod| was selected.
- (void)timeRangeSelectorViewController:
            (TimeRangeSelectorCollectionViewController*)collectionViewController
                    didSelectTimePeriod:(browsing_data::TimePeriod)timePeriod;

@end

// Table view for time range selection.
@interface TimeRangeSelectorCollectionViewController
    : SettingsRootCollectionViewController

- (instancetype)
initWithPrefs:(PrefService*)prefs
     delegate:(id<TimeRangeSelectorCollectionViewControllerDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;

- (instancetype)initWithLayout:(UICollectionViewLayout*)layout
                         style:(CollectionViewControllerStyle)style
    NS_UNAVAILABLE;

// Returns the text for the current setting, based on the values of the
// preference. Kept in this class, so that all of the code to translate from
// preference to UI is in one place.
+ (NSString*)timePeriodLabelForPrefs:(PrefService*)prefs;

@end

#endif  // IOS_CHROME_BROWSER_UI_SETTINGS_TIME_RANGE_SELECTOR_COLLECTION_VIEW_CONTROLLER_H_
