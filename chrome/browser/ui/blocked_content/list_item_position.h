// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BLOCKED_CONTENT_LIST_ITEM_POSITION_H_
#define CHROME_BROWSER_UI_BLOCKED_CONTENT_LIST_ITEM_POSITION_H_

#include <cstddef>

// This enum backs a histogram. Make sure you update enums.xml if you make
// any changes.
//
// Identifies an element's position in an ordered list. Used by both the
// framebust and popup UI on desktop platforms to indicate which element was
// clicked.
enum class ListItemPosition : int {
  kOnlyItem,
  kFirstItem,
  kMiddleItem,
  kLastItem,

  // Any new values should go before this one.
  kLast,
};

// Gets the list item position from the given distance/index and the total size
// of the collection. Distance is the measure from the beginning of the
// collection to the given element.
ListItemPosition GetListItemPositionFromDistance(size_t distance,
                                                 size_t total_size);

#endif  // CHROME_BROWSER_UI_BLOCKED_CONTENT_LIST_ITEM_POSITION_H_
