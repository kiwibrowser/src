// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_TEST_EARL_GREY_CHROME_ACTIONS_H_
#define IOS_CHROME_TEST_EARL_GREY_CHROME_ACTIONS_H_

#include <string>

#import <EarlGrey/EarlGrey.h>

namespace chrome_test_util {

// Action to longpress on element |element_id| in the Chrome's webview.
// If |triggers_context_menu| is true, this gesture is expected to
// cause the context menu to appear, and is not expected to trigger events
// in the webview. If |triggers_context_menu| is false, the converse is true.
// This action doesn't fail if the context menu isn't displayed; calling code
// should check for that separately with a matcher.
id<GREYAction> LongPressElementForContextMenu(const std::string& element_id,
                                              bool triggers_context_menu);

// Action to turn the switch of a CollectionViewSwitchCell to the given |on|
// state.
id<GREYAction> TurnCollectionViewSwitchOn(BOOL on);

// Action to turn the switch of a SyncSwitchCell to the given |on| state.
id<GREYAction> TurnSyncSwitchOn(BOOL on);

}  // namespace chrome_test_util

#endif  // IOS_CHROME_TEST_EARL_GREY_CHROME_ACTIONS_H_
