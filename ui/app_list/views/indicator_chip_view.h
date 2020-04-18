// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_VIEWS_INDICATOR_CHIP_VIEW_H_
#define UI_APP_LIST_VIEWS_INDICATOR_CHIP_VIEW_H_

#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/views/view.h"

namespace views {
class Label;
}  // namespace views

namespace app_list {

// IndicatorChipView consists of a label and is used on top of app tiles for
// indication purpose.
class IndicatorChipView : public views::View {
 public:
  explicit IndicatorChipView(const base::string16& text);
  ~IndicatorChipView() override;

  gfx::Rect GetLabelBoundsInScreen() const;

 private:
  // views::View overridden:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;

  views::View* container_ = nullptr;  // Owned by views hierarchy.
  views::Label* label_ = nullptr;     // Owned by views hierarchy.

  DISALLOW_COPY_AND_ASSIGN(IndicatorChipView);
};

}  // namespace app_list

#endif  // UI_APP_LIST_VIEWS_INDICATOR_CHIP_VIEW_H_
