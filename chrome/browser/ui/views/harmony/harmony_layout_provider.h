// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_HARMONY_HARMONY_LAYOUT_PROVIDER_H_
#define CHROME_BROWSER_UI_VIEWS_HARMONY_HARMONY_LAYOUT_PROVIDER_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/browser/ui/views/harmony/harmony_typography_provider.h"

class HarmonyLayoutProvider : public ChromeLayoutProvider {
 public:
  // The Harmony layout unit. All distances are in terms of this unit.
  static constexpr int kHarmonyLayoutUnit = 16;

  HarmonyLayoutProvider() {}
  ~HarmonyLayoutProvider() override {}

  gfx::Insets GetInsetsMetric(int metric) const override;
  int GetDistanceMetric(int metric) const override;
  views::GridLayout::Alignment GetControlLabelGridAlignment() const override;
  bool UseExtraDialogPadding() const override;
  bool ShouldShowWindowIcon() const override;
  bool IsHarmonyMode() const override;
  const views::TypographyProvider& GetTypographyProvider() const override;
  int GetSnappedDialogWidth(int min_width) const override;

 private:
  const HarmonyTypographyProvider typography_provider_;

  DISALLOW_COPY_AND_ASSIGN(HarmonyLayoutProvider);
};

#endif  // CHROME_BROWSER_UI_VIEWS_HARMONY_HARMONY_LAYOUT_PROVIDER_H_
