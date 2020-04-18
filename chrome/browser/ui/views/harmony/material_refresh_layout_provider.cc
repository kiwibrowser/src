// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/harmony/material_refresh_layout_provider.h"

#include "ui/views/layout/layout_provider.h"

int MaterialRefreshLayoutProvider::GetDistanceMetric(int metric) const {
  switch (metric) {
    case views::DistanceMetric::DISTANCE_CONTROL_VERTICAL_TEXT_PADDING:
      return 8;
  }
  return HarmonyLayoutProvider::GetDistanceMetric(metric);
}

gfx::Insets MaterialRefreshLayoutProvider::GetInsetsMetric(int metric) const {
  switch (metric) {
    case INSETS_BOOKMARKS_BAR_BUTTON:
      return gfx::Insets(5, 8);
  }
  return HarmonyLayoutProvider::GetInsetsMetric(metric);
}

int MaterialRefreshLayoutProvider::GetCornerRadiusMetric(
    views::EmphasisMetric emphasis_metric,
    const gfx::Size& size) const {
  switch (emphasis_metric) {
    case views::EMPHASIS_LOW:
      return 4;
    case views::EMPHASIS_MEDIUM:
      return 8;
    case views::EMPHASIS_HIGH:
      return std::min(size.width(), size.height()) / 2;
    default:
      NOTREACHED();
      return 0;
  }
}
