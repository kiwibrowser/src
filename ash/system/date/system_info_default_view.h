// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_DATE_SYSTEM_INFO_DEFAULT_VIEW_H_
#define ASH_SYSTEM_DATE_SYSTEM_INFO_DEFAULT_VIEW_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/views/view.h"

namespace ash {
class PowerStatusView;
class SystemTrayItem;
class TriView;

namespace tray {
class DateView;
}  // namespace tray

// The default view for the system info row in the system menu. Contains the
// current date and, if a battery is present, a string showing the current
// power status.
class ASH_EXPORT SystemInfoDefaultView : public views::View {
 public:
  explicit SystemInfoDefaultView(SystemTrayItem* owner);
  ~SystemInfoDefaultView() override;

  tray::DateView* GetDateView();
  const tray::DateView* GetDateView() const;

  // views::View:
  void Layout() override;

 private:
  friend class SystemInfoDefaultViewTest;

  // Computes and returns the width for |date_view_| so that the separator to
  // its right has the same x-position as a separator in the tiles row above.
  // Depending on the width of the date string, we align the separator with
  // either the second or third separator in the tiles row (|kMinNumTileWidths|
  // and |kMaxNumTileWidths| respectively.
  static int CalculateDateViewWidth(int preferred_width);

  tray::DateView* date_view_;

  PowerStatusView* power_status_view_ = nullptr;

  TriView* tri_view_;

  DISALLOW_COPY_AND_ASSIGN(SystemInfoDefaultView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_DATE_SYSTEM_INFO_DEFAULT_VIEW_H_
