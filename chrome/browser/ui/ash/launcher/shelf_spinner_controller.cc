// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/shelf_spinner_controller.h"

#include <memory>

#include "ash/public/cpp/shelf_model.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/shelf_spinner_item_controller.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/paint_throbber.h"

namespace {

constexpr int kUpdateIconIntervalMs = 40;  // 40ms for 25 frames per second.
constexpr int kSpinningGapPercent = 25;

class SpinningEffectSource : public gfx::CanvasImageSource {
 public:
  SpinningEffectSource(const base::WeakPtr<ShelfSpinnerController>& host,
                       const std::string& app_id,
                       const gfx::ImageSkia& image)
      : gfx::CanvasImageSource(image.size(), false /* is opaque */),
        host_(host),
        app_id_(app_id),
        image_(image) {}

  ~SpinningEffectSource() override {}

  // gfx::CanvasImageSource override.
  void Draw(gfx::Canvas* canvas) override {
    if (!host_)
      return;

    canvas->DrawImageInt(image_, 0, 0);

    const int gap = kSpinningGapPercent * image_.width() / 100;
    gfx::PaintThrobberSpinning(canvas,
                               gfx::Rect(gap, gap, image_.width() - 2 * gap,
                                         image_.height() - 2 * gap),
                               SK_ColorWHITE, host_->GetActiveTime(app_id_));
  }

 private:
  base::WeakPtr<ShelfSpinnerController> host_;
  const std::string app_id_;
  const gfx::ImageSkia image_;

  DISALLOW_COPY_AND_ASSIGN(SpinningEffectSource);
};
}  // namespace

ShelfSpinnerController::ShelfSpinnerController(ChromeLauncherController* owner)
    : owner_(owner), weak_ptr_factory_(this) {}

ShelfSpinnerController::~ShelfSpinnerController() {}

void ShelfSpinnerController::MaybeApplySpinningEffect(const std::string& app_id,
                                                      gfx::ImageSkia* image) {
  DCHECK(image);
  if (app_controller_map_.find(app_id) == app_controller_map_.end())
    return;

  const color_utils::HSL shift = {-1, 0, 0.25};
  *image = gfx::ImageSkia(
      std::make_unique<SpinningEffectSource>(
          weak_ptr_factory_.GetWeakPtr(), app_id,
          gfx::ImageSkiaOperations::CreateTransparentImage(
              gfx::ImageSkiaOperations::CreateHSLShiftedImage(*image, shift),
              0.5)),
      image->size());
}

void ShelfSpinnerController::Remove(const std::string& app_id) {
  app_controller_map_.erase(app_id);
}

void ShelfSpinnerController::Close(const std::string& app_id) {
  // Code below may invalidate passed |app_id|. Use local variable for safety.
  const std::string safe_app_id(app_id);

  AppControllerMap::const_iterator it = app_controller_map_.find(safe_app_id);
  if (it == app_controller_map_.end())
    return;

  const ash::ShelfID shelf_id(safe_app_id);
  const bool need_close_item =
      it->second == owner_->shelf_model()->GetShelfItemDelegate(shelf_id);
  app_controller_map_.erase(it);
  if (need_close_item)
    owner_->CloseLauncherItem(shelf_id);
  UpdateShelfItemIcon(safe_app_id);
}

bool ShelfSpinnerController::HasApp(const std::string& app_id) const {
  return app_controller_map_.count(app_id);
}

base::TimeDelta ShelfSpinnerController::GetActiveTime(
    const std::string& app_id) const {
  AppControllerMap::const_iterator it = app_controller_map_.find(app_id);
  if (it == app_controller_map_.end())
    return base::TimeDelta();

  return it->second->GetActiveTime();
}

Profile* ShelfSpinnerController::OwnerProfile() {
  return owner_->profile();
}

void ShelfSpinnerController::UpdateShelfItemIcon(const std::string& app_id) {
  owner_->UpdateLauncherItemImage(app_id);
}

void ShelfSpinnerController::UpdateApps() {
  if (app_controller_map_.empty())
    return;

  RegisterNextUpdate();
  for (const auto pair : app_controller_map_)
    UpdateShelfItemIcon(pair.first);
}

void ShelfSpinnerController::RegisterNextUpdate() {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ShelfSpinnerController::UpdateApps,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(kUpdateIconIntervalMs));
}

void ShelfSpinnerController::AddSpinnerToShelf(
    const std::string& app_id,
    std::unique_ptr<ShelfSpinnerItemController> controller) {
  const ash::ShelfID shelf_id(app_id);

  // We should only apply the spinner controller only over non-active items.
  const ash::ShelfItem* item = owner_->GetItem(shelf_id);
  if (item && item->status != ash::STATUS_CLOSED)
    return;

  controller->SetHost(weak_ptr_factory_.GetWeakPtr());
  ShelfSpinnerItemController* item_controller = controller.get();
  if (!item) {
    owner_->CreateAppLauncherItem(std::move(controller), ash::STATUS_RUNNING);
  } else {
    owner_->shelf_model()->SetShelfItemDelegate(shelf_id,
                                                std::move(controller));
    owner_->SetItemStatus(shelf_id, ash::STATUS_RUNNING);
  }

  if (app_controller_map_.empty())
    RegisterNextUpdate();

  app_controller_map_[app_id] = item_controller;
}
