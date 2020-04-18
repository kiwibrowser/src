// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/window_icon_util.h"

#include "ui/base/x/x11_util.h"
#include "ui/gfx/x/x11.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/x11_error_tracker.h"
#include "ui/gfx/x/x11_types.h"

gfx::ImageSkia GetWindowIcon(content::DesktopMediaID id) {
  DCHECK(id.type == content::DesktopMediaID::TYPE_WINDOW);

  Display* display = gfx::GetXDisplay();
  Atom property = gfx::GetAtom("_NET_WM_ICON");
  Atom actual_type;
  int actual_format;
  unsigned long bytes_after;  // NOLINT: type required by XGetWindowProperty
  unsigned long size;
  long* data;

  // The |error_tracker| essentially provides an empty X error handler for
  // the call of XGetWindowProperty. The motivation is to guard against crash
  // for any reason that XGetWindowProperty fails. For example, at the time that
  // XGetWindowProperty is called, the window handler (a.k.a |id.id|) may
  // already be invalid due to the fact that the end user has closed the
  // corresponding window, etc.
  std::unique_ptr<gfx::X11ErrorTracker> error_tracker(
      new gfx::X11ErrorTracker());
  int status = XGetWindowProperty(display, id.id, property, 0L, ~0L, x11::False,
                                  AnyPropertyType, &actual_type, &actual_format,
                                  &size, &bytes_after,
                                  reinterpret_cast<unsigned char**>(&data));
  error_tracker.reset();

  if (status != x11::Success) {
    return gfx::ImageSkia();
  }

  // The format of |data| is concatenation of sections like
  // [width, height, pixel data of size width * height], and the total bytes
  // number of |data| is |size|. And here we are picking the largest icon.
  int width = 0;
  int height = 0;
  int start = 0;
  int i = 0;
  while (i + 1 < static_cast<int>(size)) {
    if ((i == 0 || static_cast<int>(data[i] * data[i + 1]) > width * height) &&
        (i + 1 + data[i] * data[i + 1] < static_cast<int>(size))) {
      width = static_cast<int>(data[i]);
      height = static_cast<int>(data[i + 1]);
      start = i + 2;
    }
    i = i + 2 + static_cast<int>(data[i] * data[i + 1]);
  }

  SkBitmap result;
  SkImageInfo info = SkImageInfo::MakeN32(width, height, kUnpremul_SkAlphaType);
  result.allocPixels(info);

  uint32_t* pixels_data = reinterpret_cast<uint32_t*>(result.getPixels());

  for (long y = 0; y < height; ++y) {
    for (long x = 0; x < width; ++x) {
      pixels_data[result.rowBytesAsPixels() * y + x] =
          static_cast<uint32_t>(data[start + width * y + x]);
    }
  }

  XFree(data);
  return gfx::ImageSkia::CreateFrom1xBitmap(result);
}
