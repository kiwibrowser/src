// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/window_icon_util.h"

#include "ui/gfx/icon_util.h"

gfx::ImageSkia GetWindowIcon(content::DesktopMediaID id) {
  DCHECK(id.type == content::DesktopMediaID::TYPE_WINDOW);

  HWND hwnd = reinterpret_cast<HWND>(id.id);

  HICON icon_handle =
      reinterpret_cast<HICON>(SendMessage(hwnd, WM_GETICON, ICON_BIG, 0));

  if (!icon_handle)
    icon_handle = reinterpret_cast<HICON>(GetClassLongPtr(hwnd, GCLP_HICON));

  if (!icon_handle) {
    icon_handle =
        reinterpret_cast<HICON>(SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0));
  }

  if (!icon_handle) {
    icon_handle =
        reinterpret_cast<HICON>(SendMessage(hwnd, WM_GETICON, ICON_SMALL2, 0));
  }

  if (!icon_handle)
    icon_handle = reinterpret_cast<HICON>(GetClassLongPtr(hwnd, GCLP_HICONSM));

  if (!icon_handle)
    return gfx::ImageSkia();

  std::unique_ptr<SkBitmap> icon_bitmap(
      IconUtil::CreateSkBitmapFromHICON(icon_handle));

  if (!icon_bitmap)
    return gfx::ImageSkia();

  return gfx::ImageSkia::CreateFrom1xBitmap(*icon_bitmap);
}
