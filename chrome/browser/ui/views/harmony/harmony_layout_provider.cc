// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/harmony/harmony_layout_provider.h"

#include "ui/base/material_design/material_design_controller.h"

namespace {
constexpr int kSmallSnapPoint = 320;
constexpr int kMediumSnapPoint = 448;
constexpr int kLargeSnapPoint = 512;
}  // namespace

gfx::Insets HarmonyLayoutProvider::GetInsetsMetric(int metric) const {
  DCHECK_LT(metric, views::VIEWS_INSETS_MAX);
  switch (metric) {
    case views::INSETS_DIALOG:
    case views::INSETS_DIALOG_SUBSECTION:
      return gfx::Insets(kHarmonyLayoutUnit);
    case views::INSETS_CHECKBOX_RADIO_BUTTON: {
      gfx::Insets insets = ChromeLayoutProvider::GetInsetsMetric(metric);
      // Material Design requires that checkboxes and radio buttons are aligned
      // flush to the left edge.
      return gfx::Insets(insets.top(), 0, insets.bottom(), insets.right());
    }
    case views::INSETS_VECTOR_IMAGE_BUTTON:
      return gfx::Insets(kHarmonyLayoutUnit / 4);
    case INSETS_TOAST:
      return gfx::Insets(0, kHarmonyLayoutUnit);
    case views::InsetsMetric::INSETS_LABEL_BUTTON:
      if (ui::MaterialDesignController::IsTouchOptimizedUiEnabled())
        return gfx::Insets(kHarmonyLayoutUnit / 2, kHarmonyLayoutUnit / 2);
      return ChromeLayoutProvider::GetInsetsMetric(metric);
    default:
      return ChromeLayoutProvider::GetInsetsMetric(metric);
  }
}

int HarmonyLayoutProvider::GetDistanceMetric(int metric) const {
  DCHECK_GE(metric, views::VIEWS_INSETS_MAX);
  switch (metric) {
    case DISTANCE_CONTENT_LIST_VERTICAL_SINGLE:
      return kHarmonyLayoutUnit / 4;
    case DISTANCE_CONTENT_LIST_VERTICAL_MULTI:
      return kHarmonyLayoutUnit / 2;
    case DISTANCE_CONTROL_LIST_VERTICAL:
      return kHarmonyLayoutUnit * 3 / 4;
    case views::DISTANCE_CLOSE_BUTTON_MARGIN: {
      constexpr int kVisibleMargin = kHarmonyLayoutUnit / 2;
      // The visible margin is based on the unpadded size, so to get the actual
      // margin we need to subtract out the padding.
      return kVisibleMargin - kHarmonyLayoutUnit / 4;
    }
    case views::DISTANCE_CONTROL_VERTICAL_TEXT_PADDING:
      return kHarmonyLayoutUnit / 4;
    case views::DISTANCE_DIALOG_CONTENT_MARGIN_BOTTOM_CONTROL:
      return kHarmonyLayoutUnit * 3 / 2;
    case views::DISTANCE_DIALOG_CONTENT_MARGIN_BOTTOM_TEXT: {
      // This is reduced so there is about the same amount of visible
      // whitespace, compensating for the text's internal leading.
      return GetDistanceMetric(
                 views::DISTANCE_DIALOG_CONTENT_MARGIN_BOTTOM_CONTROL) -
             8;
    }
    case views::DISTANCE_DIALOG_CONTENT_MARGIN_TOP_CONTROL:
      return kHarmonyLayoutUnit;
    case views::DISTANCE_DIALOG_CONTENT_MARGIN_TOP_TEXT: {
      // See the comment in DISTANCE_DIALOG_CONTENT_MARGIN_BOTTOM_TEXT above.
      return GetDistanceMetric(
                 views::DISTANCE_DIALOG_CONTENT_MARGIN_TOP_CONTROL) -
             8;
    }
    case views::DISTANCE_RELATED_BUTTON_HORIZONTAL:
      return kHarmonyLayoutUnit / 2;
    case views::DISTANCE_RELATED_CONTROL_HORIZONTAL:
      return kHarmonyLayoutUnit;
    case DISTANCE_RELATED_CONTROL_HORIZONTAL_SMALL:
      return kHarmonyLayoutUnit;
    case views::DISTANCE_RELATED_CONTROL_VERTICAL:
      return kHarmonyLayoutUnit / 2;
    case DISTANCE_RELATED_CONTROL_VERTICAL_SMALL:
      return kHarmonyLayoutUnit / 2;
    case views::DISTANCE_DIALOG_BUTTON_MINIMUM_WIDTH:
    case DISTANCE_BUTTON_MINIMUM_WIDTH:
      // Minimum label size plus padding.
      return 2 * kHarmonyLayoutUnit +
             2 * GetDistanceMetric(views::DISTANCE_BUTTON_HORIZONTAL_PADDING);
    case views::DISTANCE_BUTTON_HORIZONTAL_PADDING:
      return kHarmonyLayoutUnit;
    case views::DISTANCE_BUTTON_MAX_LINKABLE_WIDTH:
      return kHarmonyLayoutUnit * 7;
    case views::DISTANCE_RELATED_LABEL_HORIZONTAL:
    case views::DISTANCE_TABLE_CELL_HORIZONTAL_MARGIN:
      return 3 * kHarmonyLayoutUnit / 4;
    case DISTANCE_RELATED_LABEL_HORIZONTAL_LIST:
      return kHarmonyLayoutUnit / 2;
    case views::DISTANCE_DIALOG_SCROLLABLE_AREA_MAX_HEIGHT:
      return kHarmonyLayoutUnit * 12;
    case DISTANCE_SUBSECTION_HORIZONTAL_INDENT:
      return 0;
    case views::DISTANCE_TEXTFIELD_HORIZONTAL_TEXT_PADDING:
      return kHarmonyLayoutUnit / 2;
    case DISTANCE_UNRELATED_CONTROL_HORIZONTAL:
      return kHarmonyLayoutUnit;
    case DISTANCE_UNRELATED_CONTROL_HORIZONTAL_LARGE:
      return kHarmonyLayoutUnit;
    case views::DISTANCE_UNRELATED_CONTROL_VERTICAL:
      return kHarmonyLayoutUnit;
    case DISTANCE_UNRELATED_CONTROL_VERTICAL_LARGE:
      return kHarmonyLayoutUnit;
    case DISTANCE_BUBBLE_PREFERRED_WIDTH:
      return kSmallSnapPoint;
    case DISTANCE_MODAL_DIALOG_PREFERRED_WIDTH:
      return kMediumSnapPoint;
    default:
      return ChromeLayoutProvider::GetDistanceMetric(metric);
  }
}

views::GridLayout::Alignment
HarmonyLayoutProvider::GetControlLabelGridAlignment() const {
  return views::GridLayout::LEADING;
}

bool HarmonyLayoutProvider::UseExtraDialogPadding() const {
  return false;
}

bool HarmonyLayoutProvider::ShouldShowWindowIcon() const {
  return false;
}

bool HarmonyLayoutProvider::IsHarmonyMode() const {
  return true;
}

int HarmonyLayoutProvider::GetSnappedDialogWidth(int min_width) const {
  for (int snap_point : {kSmallSnapPoint, kMediumSnapPoint, kLargeSnapPoint}) {
    if (min_width <= snap_point)
      return snap_point;
  }

  return ((min_width + kHarmonyLayoutUnit - 1) / kHarmonyLayoutUnit) *
         kHarmonyLayoutUnit;
}

const views::TypographyProvider& HarmonyLayoutProvider::GetTypographyProvider()
    const {
  return typography_provider_;
}
