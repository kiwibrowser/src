// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_HARMONY_MATERIAL_REFRESH_LAYOUT_PROVIDER_H_
#define CHROME_BROWSER_UI_VIEWS_HARMONY_MATERIAL_REFRESH_LAYOUT_PROVIDER_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/harmony/harmony_layout_provider.h"

class MaterialRefreshLayoutProvider : public HarmonyLayoutProvider {
 public:
  MaterialRefreshLayoutProvider() = default;
  ~MaterialRefreshLayoutProvider() override = default;

  // HarmonyLayoutProvider:
  int GetDistanceMetric(int metric) const override;
  gfx::Insets GetInsetsMetric(int metric) const override;
  int GetCornerRadiusMetric(views::EmphasisMetric emphasis_metric,
                            const gfx::Size& size = gfx::Size()) const override;
};

#endif  // CHROME_BROWSER_UI_VIEWS_HARMONY_MATERIAL_REFRESH_LAYOUT_PROVIDER_H_
