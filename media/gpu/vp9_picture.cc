// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vp9_picture.h"

namespace media {

VP9Picture::VP9Picture() = default;

VP9Picture::~VP9Picture() = default;

V4L2VP9Picture* VP9Picture::AsV4L2VP9Picture() {
  return nullptr;
}

VaapiVP9Picture* VP9Picture::AsVaapiVP9Picture() {
  return nullptr;
}

}  // namespace media
