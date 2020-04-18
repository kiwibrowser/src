// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_LAYOUT_FILL_LAYOUT_H_
#define UI_VIEWS_LAYOUT_FILL_LAYOUT_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/views/layout/layout_manager.h"
#include "ui/views/view.h"

namespace views {

///////////////////////////////////////////////////////////////////////////////
//
// FillLayout
//  A simple LayoutManager that causes the associated view's children to be
//  sized to match the bounds of its parent. The preferred size/height is
//  is calculated as the maximum values across all child views of the host.
//
///////////////////////////////////////////////////////////////////////////////
class VIEWS_EXPORT FillLayout : public LayoutManager {
 public:
  FillLayout();
  ~FillLayout() override;

  // Overridden from LayoutManager:
  void Layout(View* host) override;
  gfx::Size GetPreferredSize(const View* host) const override;
  int GetPreferredHeightForWidth(const View* host, int width) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FillLayout);
};

}  // namespace views

#endif  // UI_VIEWS_LAYOUT_FILL_LAYOUT_H_
