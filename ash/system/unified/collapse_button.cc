// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/unified/collapse_button.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/tray/tray_constants.h"
#include "ash/system/tray/tray_popup_utils.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/views/animation/flood_fill_ink_drop_ripple.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/animation/ink_drop_impl.h"
#include "ui/views/animation/ink_drop_mask.h"
#include "ui/views/border.h"

namespace ash {

namespace {

const int kCollapseIconSize = 20;

SkPath CreateCollapseButtonPath(const gfx::Rect& bounds) {
  SkPath path;
  SkScalar bottom_radius = SkIntToScalar(kTrayItemSize / 2);
  SkScalar radii[8] = {
      0, 0, 0, 0, bottom_radius, bottom_radius, bottom_radius, bottom_radius};
  path.addRoundRect(gfx::RectToSkRect(bounds), radii);
  return path;
}

// Ink drop mask that masks non-standard shape of CollapseButton.
class CollapseButtonInkDropMask : public views::InkDropMask {
 public:
  CollapseButtonInkDropMask(const gfx::Size& layer_size);

 private:
  // InkDropMask:
  void OnPaintLayer(const ui::PaintContext& context) override;

  DISALLOW_COPY_AND_ASSIGN(CollapseButtonInkDropMask);
};

CollapseButtonInkDropMask::CollapseButtonInkDropMask(
    const gfx::Size& layer_size)
    : views::InkDropMask(layer_size) {}

void CollapseButtonInkDropMask::OnPaintLayer(const ui::PaintContext& context) {
  cc::PaintFlags flags;
  flags.setAlpha(255);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setAntiAlias(true);

  ui::PaintRecorder recorder(context, layer()->size());
  recorder.canvas()->DrawPath(CreateCollapseButtonPath(layer()->bounds()),
                              flags);
}

}  // namespace

CollapseButton::CollapseButton(views::ButtonListener* listener)
    : ImageButton(listener) {
  UpdateIcon(true /* expanded */);
  SetImageAlignment(HorizontalAlignment::ALIGN_CENTER,
                    VerticalAlignment::ALIGN_BOTTOM);
  SetTooltipText(l10n_util::GetStringUTF16(IDS_ASH_STATUS_TRAY_COLLAPSE));
  SetBorder(views::CreateEmptyBorder(
      gfx::Insets((kTrayItemSize - kCollapseIconSize) / 2)));

  TrayPopupUtils::ConfigureTrayPopupButton(this);
}

CollapseButton::~CollapseButton() = default;

void CollapseButton::UpdateIcon(bool expanded) {
  gfx::ImageSkia icon =
      gfx::CreateVectorIcon(kNotificationCenterCollapseIcon, kCollapseIconSize,
                            kUnifiedMenuIconColor);
  if (!expanded)
    icon = gfx::ImageSkiaOperations::CreateRotatedImage(
        icon, SkBitmapOperations::ROTATION_180_CW);
  SetImage(views::Button::STATE_NORMAL, icon);
}

gfx::Size CollapseButton::CalculatePreferredSize() const {
  return gfx::Size(kTrayItemSize, kTrayItemSize * 3 / 2);
}

int CollapseButton::GetHeightForWidth(int width) const {
  return CalculatePreferredSize().height();
}

void CollapseButton::PaintButtonContents(gfx::Canvas* canvas) {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(kUnifiedMenuButtonColor);
  flags.setStyle(cc::PaintFlags::kFill_Style);

  canvas->DrawPath(CreateCollapseButtonPath(GetLocalBounds()), flags);

  views::ImageButton::PaintButtonContents(canvas);
}

std::unique_ptr<views::InkDrop> CollapseButton::CreateInkDrop() {
  return TrayPopupUtils::CreateInkDrop(this);
}

std::unique_ptr<views::InkDropRipple> CollapseButton::CreateInkDropRipple()
    const {
  return TrayPopupUtils::CreateInkDropRipple(
      TrayPopupInkDropStyle::FILL_BOUNDS, this,
      GetInkDropCenterBasedOnLastEvent(), kUnifiedMenuIconColor);
}

std::unique_ptr<views::InkDropHighlight>
CollapseButton::CreateInkDropHighlight() const {
  return TrayPopupUtils::CreateInkDropHighlight(
      TrayPopupInkDropStyle::FILL_BOUNDS, this, kUnifiedMenuIconColor);
}

std::unique_ptr<views::InkDropMask> CollapseButton::CreateInkDropMask() const {
  return std::make_unique<CollapseButtonInkDropMask>(size());
}

}  // namespace ash
