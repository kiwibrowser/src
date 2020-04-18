// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_image.h"

namespace gl {

bool GLImage::BindTexImageWithInternalformat(unsigned target,
                                             unsigned internalformat) {
  return false;
}

bool GLImage::EmulatingRGB() const {
  return false;
}

GLImage::Type GLImage::GetType() const {
  return Type::NONE;
}

}  // namespace gl
