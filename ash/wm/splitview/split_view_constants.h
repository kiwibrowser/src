// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_SPLITVIEW_SPLIT_VIEW_CONSTANTS_H_
#define ASH_WM_SPLITVIEW_SPLIT_VIEW_CONSTANTS_H_

#include "ash/ash_export.h"
#include "third_party/skia/include/core/SkColor.h"

namespace ash {

// The ratio between a highlight view's primary axis, and the screens
// primary axis.
ASH_EXPORT constexpr double kHighlightScreenPrimaryAxisRatio = 0.10;

// The padding between a highlight view and the edge of the screen.
ASH_EXPORT constexpr double kHighlightScreenEdgePaddingDp = 8;

// The amount of inset to be applied on a split view label. Here horizontal and
// vertical apply to the orientation before rotation (if there is rotation).
constexpr int kSplitviewLabelHorizontalInsetDp = 12;
constexpr int kSplitviewLabelVerticalInsetDp = 4;

// The preferred height of a split view label.
constexpr int kSplitviewLabelPreferredHeightDp = 24;

// The amount of round applied to the corners of a split view label.
constexpr int kSplitviewLabelRoundRectRadiusDp = 12;

// Color of split view label text.
constexpr SkColor kSplitviewLabelEnabledColor = SK_ColorWHITE;

// The color for a split view label.
constexpr SkColor kSplitviewLabelBackgroundColor =
    SkColorSetA(SK_ColorBLACK, 0xDE);

constexpr int kSplitviewAnimationDurationMs = 250;

}  // namespace ash

#endif  // ASH_WM_SPLITVIEW_SPLIT_VIEW_CONSTANTS_H_
