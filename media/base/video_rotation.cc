// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/video_rotation.h"

#include "base/logging.h"

namespace media {

std::string VideoRotationToString(VideoRotation rotation) {
  switch (rotation) {
    case VIDEO_ROTATION_0:
      return "0°";
    case VIDEO_ROTATION_90:
      return "90°";
    case VIDEO_ROTATION_180:
      return "180°";
    case VIDEO_ROTATION_270:
      return "270°";
  }
  NOTREACHED();
  return "";
}

}  // namespace media
