// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/top_icon_animation_view.h"

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/compositor/scoped_layer_animation_settings.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"

namespace app_list {

TopIconAnimationView::TopIconAnimationView(const gfx::ImageSkia& icon,
                                           const base::string16& title,
                                           const gfx::Rect& scaled_rect,
                                           bool open_folder,
                                           bool item_in_folder_icon)
    : icon_size_(kGridIconDimension, kGridIconDimension),
      icon_(new views::ImageView),
      title_(new views::Label),
      scaled_rect_(scaled_rect),
      open_folder_(open_folder),
      item_in_folder_icon_(item_in_folder_icon) {
  DCHECK(!icon.isNull());
  gfx::ImageSkia resized(gfx::ImageSkiaOperations::CreateResizedImage(
      icon,
      skia::ImageOperations::RESIZE_BEST, icon_size_));
  icon_->SetImage(resized);
  AddChildView(icon_);

  title_->SetBackgroundColor(SK_ColorTRANSPARENT);
  title_->SetAutoColorReadabilityEnabled(false);
  title_->SetHandlesTooltips(false);
  const gfx::FontList& font = AppListAppTitleFont();
  title_->SetFontList(font);
  title_->SetLineHeight(font.GetHeight());
  title_->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  title_->SetEnabledColor(SK_ColorBLACK);
  title_->SetText(title);
  if (item_in_folder_icon_) {
    // The title's opacity of the item should be changed separately if it is in
    // the folder item's icon.
    title_->SetPaintToLayer();
    title_->layer()->SetFillsBoundsOpaquely(false);
  }
  AddChildView(title_);

  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);
}

TopIconAnimationView::~TopIconAnimationView() {
  // Required due to RequiresNotificationWhenAnimatorDestroyed() returning true.
  // See ui::LayerAnimationObserver for details.
  StopObservingImplicitAnimations();
}

void TopIconAnimationView::AddObserver(TopIconAnimationObserver* observer) {
  observers_.AddObserver(observer);
}

void TopIconAnimationView::RemoveObserver(TopIconAnimationObserver* observer) {
  observers_.RemoveObserver(observer);
}

void TopIconAnimationView::TransformView() {
  // This view will delete itself on animation completion.
  set_owned_by_client();

  // Transform used for scaling down the icon and move it back inside to the
  // original folder icon. The transform's origin is this view's origin.
  gfx::Transform transform;
  transform.Translate(scaled_rect_.x() - bounds().x(),
                      scaled_rect_.y() - bounds().y());
  transform.Scale(
      static_cast<double>(scaled_rect_.width()) / bounds().width(),
      static_cast<double>(scaled_rect_.height()) / bounds().height());

  if (open_folder_) {
    // Transform to a scaled down icon inside the original folder icon.
    layer()->SetTransform(transform);
  }

  if (!item_in_folder_icon_)
    layer()->SetOpacity(open_folder_ ? 0.0f : 1.0f);

  // Animate the icon to its target location and scale when opening or
  // closing a folder.
  ui::ScopedLayerAnimationSettings settings(layer()->GetAnimator());
  settings.AddObserver(this);
  settings.SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN);
  settings.SetTransitionDuration(
      base::TimeDelta::FromMilliseconds(kFolderTransitionInDurationMs));
  layer()->SetTransform(open_folder_ ? gfx::Transform() : transform);
  if (!item_in_folder_icon_)
    layer()->SetOpacity(open_folder_ ? 1.0f : 0.0f);

  if (item_in_folder_icon_) {
    // Animate the opacity of the title.
    title_->layer()->SetOpacity(open_folder_ ? 0.0f : 1.0f);
    ui::ScopedLayerAnimationSettings title_settings(
        title_->layer()->GetAnimator());
    title_settings.SetTweenType(gfx::Tween::FAST_OUT_SLOW_IN);
    title_settings.SetTransitionDuration(
        base::TimeDelta::FromMilliseconds(kFolderTransitionInDurationMs));
    title_->layer()->SetOpacity(open_folder_ ? 1.0f : 0.0f);
  }
}

gfx::Size TopIconAnimationView::CalculatePreferredSize() const {
  return gfx::Size(kGridTileWidth, kGridTileHeight);
}

void TopIconAnimationView::Layout() {
  // This view's layout should be the same as AppListItemView's.
  gfx::Rect icon_rect(GetContentsBounds());
  icon_rect.Inset(0, kGridIconTopPadding, 0, 0);
  icon_rect.set_height(kGridIconDimension);
  icon_rect.ClampToCenteredSize(
      gfx::Size(kGridIconDimension, kGridIconDimension));
  icon_->SetBoundsRect(icon_rect);

  gfx::Rect title_rect(GetContentsBounds());
  title_rect.Inset(kGridTitleHorizontalPadding,
                   kGridIconTopPadding + kGridIconDimension + kGridTitleSpacing,
                   kGridTitleHorizontalPadding, 0);
  title_->SetBoundsRect(title_rect);
}

void TopIconAnimationView::OnImplicitAnimationsCompleted() {
  SetVisible(false);
  for (auto& observer : observers_)
    observer.OnTopIconAnimationsComplete(this);
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

bool TopIconAnimationView::RequiresNotificationWhenAnimatorDestroyed() const {
  return true;
}

}  // namespace app_list
