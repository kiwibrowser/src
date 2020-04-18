// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_HARMONY_HARMONY_TYPOGRAPHY_PROVIDER_H_
#define CHROME_BROWSER_UI_VIEWS_HARMONY_HARMONY_TYPOGRAPHY_PROVIDER_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "ui/views/style/typography_provider.h"

// TypographyProvider implementing the Harmony spec.
class HarmonyTypographyProvider : public views::TypographyProvider {
 public:
  HarmonyTypographyProvider() = default;

#if defined(OS_WIN)
  // Returns the expected platform font height for the current system
  // configuration. Different configurations can produce slightly different
  // results.
  static int GetPlatformFontHeight(int font_context);
#endif

  // TypographyProvider:
  const gfx::FontList& GetFont(int context, int style) const override;
  SkColor GetColor(const views::View& view,
                   int context,
                   int style) const override;
  int GetLineHeight(int context, int style) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(HarmonyTypographyProvider);
};

#endif  // CHROME_BROWSER_UI_VIEWS_HARMONY_HARMONY_TYPOGRAPHY_PROVIDER_H_
