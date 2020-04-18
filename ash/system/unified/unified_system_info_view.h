// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_INFO_VIEW_H_
#define ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_INFO_VIEW_H_

#include "ash/ash_export.h"
#include "ui/views/view.h"

namespace ash {

// A view at the bottom of UnifiedSystemTray bubble that shows system
// information. The view contains date, battery status, and whether the device
// is enterprise managed or not.
class ASH_EXPORT UnifiedSystemInfoView : public views::View {
 public:
  UnifiedSystemInfoView();
  ~UnifiedSystemInfoView() override;

  // views::View:
  void ChildPreferredSizeChanged(views::View* child) override;
  void ChildVisibilityChanged(views::View* child) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(UnifiedSystemInfoViewTest, EnterpriseManagedVisible);
  FRIEND_TEST_ALL_PREFIXES(UnifiedSystemInfoViewTest,
                           EnterpriseManagedVisibleForActiveDirectory);
  FRIEND_TEST_ALL_PREFIXES(UnifiedSystemInfoViewNoSessionTest,
                           SupervisedVisible);

  // EnterpriseManagedView for unit testing. Unowned.
  views::View* const enterprise_managed_;
  // SupervisedUserView for unit testing. Unowned.
  views::View* const supervised_;

  DISALLOW_COPY_AND_ASSIGN(UnifiedSystemInfoView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_UNIFIED_UNIFIED_SYSTEM_INFO_VIEW_H_
