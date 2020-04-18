// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/app/history_test_util.h"

#include "components/browsing_data/core/browsing_data_utils.h"
#import "ios/chrome/app/main_controller.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browsing_data/browsing_data_remove_mask.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/testing/wait_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace chrome_test_util {

bool ClearBrowsingHistory() {
  __block bool did_complete = false;
  [GetMainController()
      removeBrowsingDataForBrowserState:GetOriginalBrowserState()
                             timePeriod:browsing_data::TimePeriod::ALL_TIME
                             removeMask:BrowsingDataRemoveMask::REMOVE_HISTORY
                        completionBlock:^{
                          did_complete = true;
                        }];
  return testing::WaitUntilConditionOrTimeout(testing::kWaitForUIElementTimeout,
                                              ^{
                                                return did_complete;
                                              });
}

}  // namespace chrome_test_util
