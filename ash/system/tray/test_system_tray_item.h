// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TEST_SYSTEM_TRAY_ITEM_H_
#define ASH_SYSTEM_TRAY_TEST_SYSTEM_TRAY_ITEM_H_

#include "ash/system/tray/system_tray_item.h"

namespace ash {

// Trivial item implementation that tracks its views for testing. By default
// views are created and returned by the Create*View() methods and they are
// configured to be visible. Use set_has_views() and set_views_are_visible() to
// change this behavior.
class TestSystemTrayItem : public SystemTrayItem {
 public:
  TestSystemTrayItem();
  explicit TestSystemTrayItem(SystemTrayItem::UmaType uma_type);
  ~TestSystemTrayItem() override;

  void set_has_views(bool has_views) { has_views_ = has_views; }
  void set_views_are_visible(bool visible) { views_are_visible_ = visible; }

  views::View* tray_view() const { return tray_view_; }
  views::View* default_view() const { return default_view_; }
  views::View* detailed_view() const { return detailed_view_; }

  // SystemTrayItem:
  views::View* CreateTrayView(LoginStatus status) override;
  views::View* CreateDefaultView(LoginStatus status) override;
  views::View* CreateDetailedView(LoginStatus status) override;
  void OnTrayViewDestroyed() override;
  void OnDefaultViewDestroyed() override;
  void OnDetailedViewDestroyed() override;
  void UpdateAfterLoginStatusChange(LoginStatus status) override;

 private:
  bool has_views_;
  bool views_are_visible_;
  views::View* tray_view_;
  views::View* default_view_;
  views::View* detailed_view_;

  DISALLOW_COPY_AND_ASSIGN(TestSystemTrayItem);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TEST_SYSTEM_TRAY_ITEM_H_
