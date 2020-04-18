// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"

#include <algorithm>

#include "base/logging.h"
#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "chrome/browser/ui/views/harmony/harmony_layout_provider.h"
#include "chrome/browser/ui/views/harmony/material_refresh_layout_provider.h"
#include "ui/base/material_design/material_design_controller.h"

namespace {

ChromeLayoutProvider* g_chrome_layout_provider = nullptr;

}  // namespace

ChromeLayoutProvider::ChromeLayoutProvider() {
  DCHECK_EQ(nullptr, g_chrome_layout_provider);
  g_chrome_layout_provider = this;
}

ChromeLayoutProvider::~ChromeLayoutProvider() {
  DCHECK_EQ(this, g_chrome_layout_provider);
  g_chrome_layout_provider = nullptr;
}

// static
ChromeLayoutProvider* ChromeLayoutProvider::Get() {
  // Check to avoid downcasting a base LayoutProvider.
  DCHECK_EQ(g_chrome_layout_provider, views::LayoutProvider::Get());
  return static_cast<ChromeLayoutProvider*>(views::LayoutProvider::Get());
}

// static
std::unique_ptr<views::LayoutProvider>
ChromeLayoutProvider::CreateLayoutProvider() {
  if (ui::MaterialDesignController::IsRefreshUi())
    return std::make_unique<MaterialRefreshLayoutProvider>();
  return ui::MaterialDesignController::IsSecondaryUiMaterial()
             ? std::make_unique<HarmonyLayoutProvider>()
             : std::make_unique<ChromeLayoutProvider>();
}

gfx::Insets ChromeLayoutProvider::GetInsetsMetric(int metric) const {
  switch (metric) {
    case ChromeInsetsMetric::INSETS_OMNIBOX:
      return gfx::Insets(3);
    case ChromeInsetsMetric::INSETS_TOAST:
      return gfx::Insets(0, 8);
    case INSETS_BOOKMARKS_BAR_BUTTON:
      if (ui::MaterialDesignController::IsTouchOptimizedUiEnabled())
        return gfx::Insets(8, 12);
      return GetInsetsMetric(views::InsetsMetric::INSETS_LABEL_BUTTON);
    default:
      return views::LayoutProvider::GetInsetsMetric(metric);
  }
}

int ChromeLayoutProvider::GetDistanceMetric(int metric) const {
  switch (metric) {
    case DISTANCE_BUTTON_MINIMUM_WIDTH:
      return 48;
    case DISTANCE_CONTENT_LIST_VERTICAL_SINGLE:
      return 4;
    case DISTANCE_CONTENT_LIST_VERTICAL_MULTI:
      return 8;
    case DISTANCE_CONTROL_LIST_VERTICAL:
      return GetDistanceMetric(views::DISTANCE_RELATED_CONTROL_VERTICAL);
    case DISTANCE_RELATED_CONTROL_HORIZONTAL_SMALL:
      return 8;
    case DISTANCE_RELATED_CONTROL_VERTICAL_SMALL:
      return 4;
    case DISTANCE_RELATED_LABEL_HORIZONTAL_LIST:
      return 8;
    case DISTANCE_SUBSECTION_HORIZONTAL_INDENT:
      return 10;
    case DISTANCE_UNRELATED_CONTROL_HORIZONTAL:
      return 12;
    case DISTANCE_UNRELATED_CONTROL_HORIZONTAL_LARGE:
      return 20;
    case DISTANCE_UNRELATED_CONTROL_VERTICAL_LARGE:
      return 30;
    case DISTANCE_TOAST_CONTROL_VERTICAL:
      return 8;
    case DISTANCE_TOAST_LABEL_VERTICAL:
      return 12;
    case DISTANCE_MODAL_DIALOG_PREFERRED_WIDTH:
      return 400;
    case DISTANCE_BUBBLE_PREFERRED_WIDTH:
      return 320;
    default:
      return views::LayoutProvider::GetDistanceMetric(metric);
  }
}

const views::TypographyProvider& ChromeLayoutProvider::GetTypographyProvider()
    const {
  // This is not a data member because then HarmonyLayoutProvider would inherit
  // it, even when it provides its own.
  CR_DEFINE_STATIC_LOCAL(LegacyTypographyProvider, legacy_provider, ());
  return legacy_provider;
}

views::GridLayout::Alignment
ChromeLayoutProvider::GetControlLabelGridAlignment() const {
  return views::GridLayout::TRAILING;
}

bool ChromeLayoutProvider::UseExtraDialogPadding() const {
  return true;
}

bool ChromeLayoutProvider::ShouldShowWindowIcon() const {
  return true;
}

bool ChromeLayoutProvider::IsHarmonyMode() const {
  return false;
}
