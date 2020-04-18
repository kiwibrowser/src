// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_H_

#include <vector>

#include "ui/gfx/geometry/size.h"

namespace gfx {
class Rect;
}

struct TabSizeInfo {
  // The width of pinned tabs.
  int pinned_tab_width;

  // The min width of active/inactive tabs.
  int min_active_width;
  int min_inactive_width;

  // The max size of tabs. Active and inactive tabs have the same max width.
  gfx::Size max_size;

  // The overlap between adjacent tabs. When positioning tabs the x-coordinate
  // of a tab is calculated as the x-coordinate of the previous tab plus the
  // previous tab's width minus the |tab_overlap|, e.g.
  // next_tab_x = previous_tab.max_x() - tab_overlap.
  int tab_overlap;

  // Additional offset between the last pinned tab and the first normal tab.
  int pinned_to_normal_offset;
};

// Calculates the bounds of the pinned tabs. This assumes |tabs_bounds| is the
// same size as |num_tabs|. Returns the x-coordinate to use for the first
// non-pinned tab, if any.
int CalculateBoundsForPinnedTabs(const TabSizeInfo& tab_size_info,
                                 int num_pinned_tabs,
                                 int num_tabs,
                                 int start_x,
                                 std::vector<gfx::Rect>* tabs_bounds);

// Calculates and returns the bounds of the tabs. |width| is the available
// width to use for tab layout. This never sizes the tabs smaller then the
// minimum widths in TabSizeInfo, and as a result the calculated bounds may go
// beyond |width|.
std::vector<gfx::Rect> CalculateBounds(const TabSizeInfo& tab_size_info,
                                       int num_pinned_tabs,
                                       int num_tabs,
                                       int active_index,
                                       int start_x,
                                       int width,
                                       int* active_width,
                                       int* inactive_width);

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_TAB_STRIP_LAYOUT_H_
