// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/draw_properties.h"

namespace cc {

DrawProperties::DrawProperties()
    : opacity(0.f),
      screen_space_transform_is_animating(false),
      is_clipped(false) {}

DrawProperties::~DrawProperties() = default;

}  // namespace cc
