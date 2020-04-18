// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/feature_promos/incognito_window_promo_bubble_view.h"

#include "base/strings/string_number_conversions.h"
#include "chrome/grit/generated_resources.h"
#include "components/variations/variations_associated_data.h"

// static
IncognitoWindowPromoBubbleView* IncognitoWindowPromoBubbleView::CreateOwned(
    views::View* anchor_view) {
  return new IncognitoWindowPromoBubbleView(anchor_view);
}

IncognitoWindowPromoBubbleView::IncognitoWindowPromoBubbleView(
    views::View* anchor_view)
    : FeaturePromoBubbleView(anchor_view,
                             views::BubbleBorder::TOP_RIGHT,
                             GetStringSpecifier(),
                             ActivationAction::ACTIVATE) {}

IncognitoWindowPromoBubbleView::~IncognitoWindowPromoBubbleView() = default;

int IncognitoWindowPromoBubbleView::GetStringSpecifier() {
  static constexpr int kTextIds[] = {
      IDS_INCOGNITOWINDOW_PROMO_0, IDS_INCOGNITOWINDOW_PROMO_1,
      IDS_INCOGNITOWINDOW_PROMO_2, IDS_INCOGNITOWINDOW_PROMO_3};
  const std::string& str = variations::GetVariationParamValue(
      "IncognitoWindowInProductHelp", "x_promo_string");
  size_t text_specifier;
  if (!base::StringToSizeT(str, &text_specifier) ||
      text_specifier >= arraysize(kTextIds)) {
    text_specifier = 0;
  }

  return kTextIds[text_specifier];
}
