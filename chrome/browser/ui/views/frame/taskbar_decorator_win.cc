// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/taskbar_decorator_win.h"

#include <objbase.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/task_scheduler/post_task.h"
#include "base/win/scoped_gdi_object.h"
#include "chrome/browser/profiles/profile_avatar_icon_util.h"
#include "skia/ext/image_operations.h"
#include "skia/ext/platform_canvas.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image.h"
#include "ui/views/win/hwnd_util.h"

namespace chrome {

namespace {

// Responsible for invoking TaskbarList::SetOverlayIcon(). The call to
// TaskbarList::SetOverlayIcon() runs a nested run loop that proves
// problematic when called on the UI thread. Additionally it seems the call may
// take a while to complete. For this reason we call it on a worker thread.
//
// Docs for TaskbarList::SetOverlayIcon() say it does nothing if the HWND is not
// valid.
void SetOverlayIcon(HWND hwnd, std::unique_ptr<SkBitmap> bitmap) {
  Microsoft::WRL::ComPtr<ITaskbarList3> taskbar;
  HRESULT result = ::CoCreateInstance(
      CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskbar));
  if (FAILED(result) || FAILED(taskbar->HrInit()))
    return;

  base::win::ScopedGDIObject<HICON> icon;
  if (bitmap.get()) {
    DCHECK_GE(bitmap.get()->width(), bitmap.get()->height());
    // Maintain aspect ratio on resize.
    const int kOverlayIconSize = 16;
    int resized_height =
        bitmap.get()->height() * kOverlayIconSize / bitmap.get()->width();
    DCHECK_GE(kOverlayIconSize, resized_height);
    // Since the target size is so small, we use our best resizer.
    SkBitmap sk_icon = skia::ImageOperations::Resize(
        *bitmap.get(),
        skia::ImageOperations::RESIZE_LANCZOS3,
        kOverlayIconSize, resized_height);

    // Paint the resized icon onto a 16x16 canvas otherwise Windows will badly
    // hammer it to 16x16.
    SkBitmap offscreen_bitmap;
    offscreen_bitmap.allocN32Pixels(kOverlayIconSize, kOverlayIconSize);
    SkCanvas offscreen_canvas(offscreen_bitmap);
    offscreen_canvas.clear(SK_ColorTRANSPARENT);
    offscreen_canvas.drawBitmap(sk_icon, 0, kOverlayIconSize - resized_height);
    icon = IconUtil::CreateHICONFromSkBitmap(offscreen_bitmap);
    if (!icon.is_valid())
      return;
  }
  taskbar->SetOverlayIcon(hwnd, icon.get(), L"");
}

}  // namespace

void DrawTaskbarDecoration(gfx::NativeWindow window, const gfx::Image* image) {
  HWND hwnd = views::HWNDForNativeWindow(window);

  // SetOverlayIcon() does nothing if the window is not visible so testing here
  // avoids all the wasted effort of the image resizing.
  if (!::IsWindowVisible(hwnd))
    return;

  // Copy the image since we're going to use it on a separate thread and
  // gfx::Image isn't thread safe.
  std::unique_ptr<SkBitmap> bitmap;
  if (image) {
    bitmap.reset(new SkBitmap(
        profiles::GetAvatarIconAsSquare(*image->ToSkBitmap(), 1)));
  }
  base::CreateCOMSTATaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE})
      ->PostTask(FROM_HERE,
                 base::Bind(&SetOverlayIcon, hwnd, base::Passed(&bitmap)));
}

}  // namespace chrome
