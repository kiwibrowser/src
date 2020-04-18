// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/harmony/bulleted_label_list_view.h"

#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/grid_layout.h"

namespace {
constexpr int kColumnSetId = 0;

class BulletView : public views::View {
 public:
  explicit BulletView(SkColor color) : color_(color) {}

  void OnPaint(gfx::Canvas* canvas) override;

 private:
  SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(BulletView);
};

void BulletView::OnPaint(gfx::Canvas* canvas) {
  View::OnPaint(canvas);

  SkScalar radius = std::min(height(), width()) / 8.0;
  gfx::Point center = GetLocalBounds().CenterPoint();

  SkPath path;
  path.addCircle(center.x(), center.y(), radius);

  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kStrokeAndFill_Style);
  flags.setColor(color_);
  flags.setAntiAlias(true);

  canvas->DrawPath(path, flags);
}

}  // namespace

BulletedLabelListView::BulletedLabelListView()
    : BulletedLabelListView(std::vector<base::string16>()) {}

BulletedLabelListView::BulletedLabelListView(
    const std::vector<base::string16>& texts) {
  constexpr auto FILL = views::GridLayout::FILL;
  constexpr auto USE_PREF = views::GridLayout::USE_PREF;
  constexpr auto FIXED = views::GridLayout::FIXED;

  views::GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));
  views::ColumnSet* columns = layout->AddColumnSet(kColumnSetId);

  int width = ChromeLayoutProvider::Get()->GetDistanceMetric(
      DISTANCE_UNRELATED_CONTROL_HORIZONTAL);
  columns->AddColumn(FILL, FILL, 0, FIXED, width, width);
  columns->AddColumn(FILL, FILL, 1, USE_PREF, 0, 0);

  for (const auto& text : texts)
    AddLabel(text);
}

BulletedLabelListView::~BulletedLabelListView() {}

void BulletedLabelListView::AddLabel(const base::string16& text) {
  views::GridLayout* layout =
      static_cast<views::GridLayout*>(GetLayoutManager());
  layout->StartRow(0, kColumnSetId);

  auto label = std::make_unique<views::Label>(text);

  label->SetMultiLine(true);
  label->SetHorizontalAlignment(gfx::ALIGN_LEFT);

  layout->AddView(new BulletView(label->enabled_color()));
  layout->AddView(label.release());
}
