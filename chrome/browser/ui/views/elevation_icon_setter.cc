// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/elevation_icon_setter.h"

#include "base/callback.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "ui/views/controls/button/label_button.h"

#if defined(OS_WIN)
#include <windows.h>
#include <shellapi.h>

#include "base/task_runner_util.h"
#include "base/win/win_util.h"
#include "ui/display/win/dpi.h"
#include "ui/gfx/icon_util.h"
#endif


// Helpers --------------------------------------------------------------------

namespace {

#if defined(OS_WIN)
std::unique_ptr<SkBitmap> GetElevationIcon() {
  std::unique_ptr<SkBitmap> icon;
  if (!base::win::UserAccountControlIsEnabled())
    return icon;

  SHSTOCKICONINFO icon_info = { sizeof(SHSTOCKICONINFO) };
  typedef HRESULT (STDAPICALLTYPE *GetStockIconInfo)(SHSTOCKICONID,
                                                     UINT,
                                                     SHSTOCKICONINFO*);
  // Even with the runtime guard above, we have to use GetProcAddress()
  // here, because otherwise the loader will try to resolve the function
  // address on startup, which will break on XP.
  GetStockIconInfo func = reinterpret_cast<GetStockIconInfo>(
      GetProcAddress(GetModuleHandle(L"shell32.dll"), "SHGetStockIconInfo"));
  // TODO(pkasting): Run on a background thread since this call spins a nested
  // message loop.
  if (FAILED((*func)(SIID_SHIELD, SHGSI_ICON | SHGSI_SMALLICON, &icon_info)))
    return icon;

  icon.reset(IconUtil::CreateSkBitmapFromHICON(
      icon_info.hIcon,
      gfx::Size(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON))));
  DestroyIcon(icon_info.hIcon);
  return icon;
}
#endif

}  // namespace


// ElevationIconSetter --------------------------------------------------------

ElevationIconSetter::ElevationIconSetter(views::LabelButton* button,
                                         base::OnceClosure callback)
    : button_(button), weak_factory_(this) {
#if defined(OS_WIN)
  base::PostTaskAndReplyWithResult(
      base::CreateCOMSTATaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING})
          .get(),
      FROM_HERE, base::BindOnce(&GetElevationIcon),
      base::BindOnce(&ElevationIconSetter::SetButtonIcon,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
#endif
}

ElevationIconSetter::~ElevationIconSetter() {
}

void ElevationIconSetter::SetButtonIcon(base::OnceClosure callback,
                                        std::unique_ptr<SkBitmap> icon) {
  if (icon) {
    float device_scale_factor = 1.0f;
#if defined(OS_WIN)
    // Windows gives us back a correctly-scaled image for the current DPI, so
    // mark this image as having been scaled for the current DPI already.
    device_scale_factor = display::win::GetDPIScale();
#endif
    button_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::ImageSkia(gfx::ImageSkiaRep(*icon, device_scale_factor)));
    button_->SizeToPreferredSize();
    if (button_->parent())
      button_->parent()->Layout();
    if (!callback.is_null())
      std::move(callback).Run();
  }
}
