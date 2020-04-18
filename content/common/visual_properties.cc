// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/visual_properties.h"

namespace content {

VisualProperties::VisualProperties() = default;

VisualProperties::VisualProperties(const VisualProperties& other) = default;

VisualProperties::~VisualProperties() = default;

VisualProperties& VisualProperties::operator=(const VisualProperties& other) =
    default;

}  // namespace content
