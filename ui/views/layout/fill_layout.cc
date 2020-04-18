// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/layout/fill_layout.h"

#include <algorithm>

namespace views {

FillLayout::FillLayout() {}

FillLayout::~FillLayout() {}

void FillLayout::Layout(View* host) {
  if (!host->has_children())
    return;

  for (int i = 0; i < host->child_count(); ++i)
    host->child_at(i)->SetBoundsRect(host->GetContentsBounds());
}

gfx::Size FillLayout::GetPreferredSize(const View* host) const {
  if (!host->has_children())
    return gfx::Size();

  gfx::Size preferred_size;
  for (int i = 0; i < host->child_count(); ++i)
    preferred_size.SetToMax(host->child_at(i)->GetPreferredSize());
  gfx::Rect rect(preferred_size);
  rect.Inset(-host->GetInsets());
  return rect.size();
}

int FillLayout::GetPreferredHeightForWidth(const View* host, int width) const {
  if (!host->has_children())
    return 0;

  const gfx::Insets insets = host->GetInsets();
  int preferred_height = 0;
  for (int i = 0; i < host->child_count(); ++i) {
    int cur_preferred_height = 0;
    cur_preferred_height =
        host->child_at(i)->GetHeightForWidth(width - insets.width()) +
        insets.height();
    preferred_height = std::max(preferred_height, cur_preferred_height);
  }
  return preferred_height;
}

}  // namespace views
