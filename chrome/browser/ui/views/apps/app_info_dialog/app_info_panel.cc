// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/apps/app_info_dialog/app_info_panel.h"

#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace {

// The spacing between the key and the value labels in the Details section.
const int kSpacingBetweenKeyAndStartOfValue = 3;
}

AppInfoPanel::AppInfoPanel(Profile* profile, const extensions::Extension* app)
    : profile_(profile), app_(app) {
}

AppInfoPanel::~AppInfoPanel() {
}

void AppInfoPanel::Close() {
  GetWidget()->Close();
}

void AppInfoPanel::OpenLink(const GURL& url) {
  DCHECK(!url.is_empty());
  NavigateParams params(profile_, url, ui::PAGE_TRANSITION_LINK);
  Navigate(&params);
}

views::Label* AppInfoPanel::CreateHeading(const base::string16& text) const {
  views::Label* label = new views::Label(text);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  label->SetFontList(ui::ResourceBundle::GetSharedInstance().GetFontList(
      ui::ResourceBundle::MediumFont));
  return label;
}

views::View* AppInfoPanel::CreateVerticalStack(int child_spacing) const {
  views::View* vertically_stacked_view = new views::View();
  vertically_stacked_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kVertical, gfx::Insets(), child_spacing));
  return vertically_stacked_view;
}

views::View* AppInfoPanel::CreateVerticalStack() const {
  return CreateVerticalStack(ChromeLayoutProvider::Get()->GetDistanceMetric(
      views::DISTANCE_RELATED_CONTROL_VERTICAL));
}

views::View* AppInfoPanel::CreateHorizontalStack(int child_spacing) const {
  views::View* vertically_stacked_view = new views::View();
  vertically_stacked_view->SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, gfx::Insets(), child_spacing));
  return vertically_stacked_view;
}

views::View* AppInfoPanel::CreateKeyValueField(views::View* key,
                                               views::View* value) const {
  views::View* horizontal_stack =
      CreateHorizontalStack(kSpacingBetweenKeyAndStartOfValue);
  horizontal_stack->AddChildView(key);
  horizontal_stack->AddChildView(value);
  return horizontal_stack;
}
