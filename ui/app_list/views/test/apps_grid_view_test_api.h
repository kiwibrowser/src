// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_TEST_APPS_GRID_VIEW_TEST_API_H_
#define UI_APP_LIST_VIEWS_TEST_APPS_GRID_VIEW_TEST_API_H_

#include "base/macros.h"

namespace gfx {
class Rect;
}

namespace views {
class View;
}

namespace app_list {

class AppsGridView;

namespace test {

class AppsGridViewTestApi {
 public:
  explicit AppsGridViewTestApi(AppsGridView* view);
  ~AppsGridViewTestApi();

  views::View* GetViewAtModelIndex(int index) const;

  void LayoutToIdealBounds();

  gfx::Rect GetItemTileRectOnCurrentPageAt(int row, int col) const;

  void PressItemAt(int index);

  bool HasPendingPageFlip() const;

  int TilesPerPage(int page) const;

 private:
  AppsGridView* view_;

  DISALLOW_COPY_AND_ASSIGN(AppsGridViewTestApi);
};

}  // namespace test
}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_TEST_APPS_GRID_VIEW_TEST_API_H_
