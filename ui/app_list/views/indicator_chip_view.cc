// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/indicator_chip_view.h"

#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/fill_layout.h"

namespace app_list {

namespace {

constexpr int kIndicatorHeight = 16;
constexpr int kHorizontalPadding = 16;
constexpr int kBorderCornerRadius = 8;

constexpr SkColor kLabelColor = SkColorSetARGB(0x8A, 0xFF, 0xFF, 0xFF);
constexpr SkColor kBackgroundColor = SkColorSetARGB(0x14, 0xFF, 0xFF, 0xFF);

class IndicatorBackground : public views::Background {
 public:
  IndicatorBackground() = default;
  ~IndicatorBackground() override = default;

 private:
  // views::Background overrides:
  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setColor(kBackgroundColor);
    flags.setAntiAlias(true);
    canvas->DrawRoundRect(view->GetContentsBounds(), kBorderCornerRadius,
                          flags);
  }

  DISALLOW_COPY_AND_ASSIGN(IndicatorBackground);
};

}  // namespace

IndicatorChipView::IndicatorChipView(const base::string16& text) {
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  container_ = new views::View();
  container_->set_can_process_events_within_subtree(false);
  AddChildView(container_);

  container_->SetBackground(std::make_unique<IndicatorBackground>());

  label_ = new views::Label(text);
  const gfx::FontList& small_font =
      ui::ResourceBundle::GetSharedInstance().GetFontList(
          ui::ResourceBundle::SmallFont);
  label_->SetFontList(small_font.DeriveWithSizeDelta(-2));
  label_->SetEnabledColor(kLabelColor);
  label_->SetAutoColorReadabilityEnabled(false);
  label_->SetBackgroundColor(kBackgroundColor);
  label_->SetHandlesTooltips(false);
  label_->SetHorizontalAlignment(gfx::ALIGN_CENTER);

  container_->AddChildView(label_);
}

IndicatorChipView::~IndicatorChipView() = default;

gfx::Rect IndicatorChipView::GetLabelBoundsInScreen() const {
  return label_ ? label_->GetBoundsInScreen() : gfx::Rect();
}

gfx::Size IndicatorChipView::CalculatePreferredSize() const {
  const int label_width = label_->GetPreferredSize().width();
  return gfx::Size(label_width + 2 * kHorizontalPadding, kIndicatorHeight);
}

void IndicatorChipView::Layout() {
  const int label_width = label_->GetPreferredSize().width();
  container_->SetSize(
      gfx::Size(label_width + 2 * kHorizontalPadding, kIndicatorHeight));
  label_->SetBoundsRect(container_->GetContentsBounds());
}

}  // namespace app_list
