// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tab_icon_view.h"

#include <memory>

#if defined(OS_WIN)
#include <windows.h>
#include <shellapi.h>
#endif

#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/views/tab_icon_view_model.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/paint_throbber.h"
#include "ui/native_theme/native_theme.h"

#if defined(OS_WIN)
#include "chrome/browser/win/app_icon.h"
#include "ui/gfx/icon_util.h"
#endif

namespace {

bool g_initialized = false;
gfx::ImageSkia* g_default_favicon = nullptr;

}  // namespace

// static
void TabIconView::InitializeIfNeeded() {
  if (!g_initialized) {
    g_initialized = true;

#if defined(OS_WIN)
    // The default window icon is the application icon, not the default
    // favicon.
    HICON app_icon = GetAppIcon();
    std::unique_ptr<SkBitmap> bitmap(
        IconUtil::CreateSkBitmapFromHICON(app_icon, gfx::Size(16, 16)));
    g_default_favicon = new gfx::ImageSkia(gfx::ImageSkiaRep(*bitmap, 1.0f));
    DestroyIcon(app_icon);
#else
    ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    g_default_favicon = rb.GetImageSkiaNamed(IDR_PRODUCT_LOGO_16);
#endif
  }
}

TabIconView::TabIconView(TabIconViewModel* model,
                         views::MenuButtonListener* listener)
    : views::MenuButton(base::string16(), listener, false),
      model_(model),
      is_light_(false) {
  InitializeIfNeeded();
  // Inheriting from Button causes this View to be focusable, but it us
  // purely decorative and should not be exposed as focusable in accessibility.
  SetFocusBehavior(FocusBehavior::NEVER);
}

TabIconView::~TabIconView() {
}

void TabIconView::Update() {
  if (!model_->ShouldTabIconViewAnimate())
    throbber_start_time_ = base::TimeTicks();

  SchedulePaint();
}

void TabIconView::PaintThrobber(gfx::Canvas* canvas) {
  if (throbber_start_time_ == base::TimeTicks())
    throbber_start_time_ = base::TimeTicks::Now();

  gfx::PaintThrobberSpinning(
      canvas, GetLocalBounds(),
      GetNativeTheme()->GetSystemColor(
          is_light_ ? ui::NativeTheme::kColorId_ThrobberLightColor
                    : ui::NativeTheme::kColorId_ThrobberSpinningColor),
      base::TimeTicks::Now() - throbber_start_time_);
}

void TabIconView::PaintFavicon(gfx::Canvas* canvas,
                               const gfx::ImageSkia& image) {
  // For source images smaller than the favicon square, scale them as if they
  // were padded to fit the favicon square, so we don't blow up tiny favicons
  // into larger or nonproportional results.
  float float_src_w = static_cast<float>(image.width());
  float float_src_h = static_cast<float>(image.height());
  float scalable_w, scalable_h;
  if (image.width() <= gfx::kFaviconSize &&
      image.height() <= gfx::kFaviconSize) {
    scalable_w = scalable_h = gfx::kFaviconSize;
  } else {
    scalable_w = float_src_w;
    scalable_h = float_src_h;
  }

  // Scale proportionately.
  float scale = std::min(static_cast<float>(width()) / scalable_w,
                         static_cast<float>(height()) / scalable_h);
  int dest_w = static_cast<int>(float_src_w * scale);
  int dest_h = static_cast<int>(float_src_h * scale);

  // Center the scaled image.
  canvas->DrawImageInt(image, 0, 0, image.width(), image.height(),
                       (width() - dest_w) / 2, (height() - dest_h) / 2, dest_w,
                       dest_h, true);
}

gfx::Size TabIconView::CalculatePreferredSize() const {
  return gfx::Size(gfx::kFaviconSize, gfx::kFaviconSize);
}

const char* TabIconView::GetClassName() const {
  return "TabIconView";
}

void TabIconView::PaintButtonContents(gfx::Canvas* canvas) {
  bool rendered = false;

  if (model_->ShouldTabIconViewAnimate()) {
    rendered = true;
    PaintThrobber(canvas);
  } else {
    gfx::ImageSkia favicon = model_->GetFaviconForTabIconView();
    if (!favicon.isNull()) {
      rendered = true;
      PaintFavicon(canvas, favicon);
    }
  }

  if (!rendered)
    PaintFavicon(canvas, *g_default_favicon);
}
