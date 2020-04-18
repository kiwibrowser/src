// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/arc_app_window.h"

#include "ash/public/cpp/app_list/app_list_constants.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_icon.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/ash/launcher/arc_app_window_launcher_controller.h"
#include "chrome/browser/ui/ash/launcher/arc_app_window_launcher_item_controller.h"
#include "components/exo/shell_surface_base.h"
#include "extensions/common/constants.h"
#include "ui/aura/window.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/widget/native_widget_aura.h"
#include "ui/views/widget/widget.h"

ArcAppWindow::ArcAppWindow(int task_id,
                           const arc::ArcAppShelfId& app_shelf_id,
                           views::Widget* widget,
                           ArcAppWindowLauncherController* owner,
                           Profile* profile)
    : AppWindowBase(ash::ShelfID(app_shelf_id.app_id()), widget),
      task_id_(task_id),
      app_shelf_id_(app_shelf_id),
      owner_(owner),
      profile_(profile) {
  SetDefaultAppIcon();
}

ArcAppWindow::~ArcAppWindow() {
  ImageDecoder::Cancel(this);
}

void ArcAppWindow::SetFullscreenMode(FullScreenMode mode) {
  DCHECK(mode != FullScreenMode::NOT_DEFINED);
  fullscreen_mode_ = mode;
}

void ArcAppWindow::SetDescription(
    const std::string& title,
    const std::vector<uint8_t>& unsafe_icon_data_png) {
  if (!title.empty())
    GetNativeWindow()->SetTitle(base::UTF8ToUTF16(title));
  ImageDecoder::Cancel(this);
  if (unsafe_icon_data_png.empty()) {
    // Reset custom icon. Switch back to default.
    SetDefaultAppIcon();
    return;
  }

  if (ArcAppIcon::IsSafeDecodingDisabledForTesting()) {
    SkBitmap bitmap;
    if (gfx::PNGCodec::Decode(&unsafe_icon_data_png[0],
                              unsafe_icon_data_png.size(), &bitmap)) {
      OnImageDecoded(bitmap);
    } else {
      OnDecodeImageFailed();
    }
  } else {
    ImageDecoder::Start(this, unsafe_icon_data_png);
  }
}

bool ArcAppWindow::IsActive() const {
  return widget()->IsActive() && owner_->active_task_id() == task_id_;
}

void ArcAppWindow::Close() {
  arc::CloseTask(task_id_);
}

void ArcAppWindow::OnIconUpdated(ArcAppIcon* icon) {
  SetIcon(icon->image_skia());
}

void ArcAppWindow::SetDefaultAppIcon() {
  if (!app_icon_) {
    app_icon_ = std::make_unique<ArcAppIcon>(
        profile_, app_shelf_id_.ToString(), app_list::kGridIconDimension, this);
  }
  // Apply default image now and in case icon is updated then OnIconUpdated()
  // will be called additionally.
  OnIconUpdated(app_icon_.get());
}

void ArcAppWindow::SetIcon(const gfx::ImageSkia& icon) {
  if (!exo::ShellSurfaceBase::GetMainSurface(GetNativeWindow())) {
    // Support unit tests where we don't have exo system initialized.
    views::NativeWidgetAura::AssignIconToAuraWindow(
        GetNativeWindow(), gfx::ImageSkia() /* window_icon */,
        icon /* app_icon */);
    return;
  }
  exo::ShellSurfaceBase* shell_surface = static_cast<exo::ShellSurfaceBase*>(
      widget()->widget_delegate()->GetContentsView());
  if (!shell_surface)
    return;
  shell_surface->SetIcon(icon);
}

void ArcAppWindow::OnImageDecoded(const SkBitmap& decoded_image) {
  // Use the custom icon and stop observing updates.
  app_icon_.reset();
  SetIcon(gfx::ImageSkiaOperations::CreateResizedImage(
      gfx::ImageSkia(gfx::ImageSkiaRep(decoded_image, 1.0f)),
      skia::ImageOperations::RESIZE_BEST,
      gfx::Size(extension_misc::EXTENSION_ICON_SMALL,
                extension_misc::EXTENSION_ICON_SMALL)));
}
