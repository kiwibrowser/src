// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/window_icon_util.h"

#include "content/public/browser/desktop_media_id.h"
#include "ui/aura/client/aura_constants.h"

gfx::ImageSkia GetWindowIcon(content::DesktopMediaID id) {
  DCHECK_EQ(content::DesktopMediaID::TYPE_WINDOW, id.type);
  // TODO(tonikitoo): can we make the implementation of
  // chrome/browser/media/webrtc/window_icon_util_chromeos.cc generic
  // enough so we can reuse it here?
  NOTIMPLEMENTED();
  return gfx::ImageSkia();
}
