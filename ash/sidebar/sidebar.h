// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SIDEBAR_SIDEBAR_H_
#define ASH_SIDEBAR_SIDEBAR_H_

#include <memory>

#include "ash/sidebar/sidebar_params.h"
#include "base/macros.h"

namespace ash {

class Shelf;
class SidebarWidget;

/**
 * Class to handle and manage the sidebar and its components (eg. window and
 * widget).
 *
 * This class is used as an interface of Sidebar for external components.
 */
class Sidebar {
 public:
  Sidebar();
  virtual ~Sidebar();

  void SetShelf(Shelf* shelf);
  void Show(SidebarInitMode mode);
  void Hide();
  bool IsVisible() const;

 private:
  // The widget of Sidebar, which has the views of Sidebar. Not owned by this,
  // but by its native widget (NATIVE_WIDGET_OWNS_WIDGET).
  SidebarWidget* widget_ = nullptr;

  // The shelf associated with the sidebar widget. This is the value set by
  // SetRootAndShelf(). Not owned by this.
  Shelf* shelf_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Sidebar);
};

}  // namespace ash

#endif  // ASH_SIDEBAR_SIDEBAR_H_
