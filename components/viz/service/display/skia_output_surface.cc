// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display/skia_output_surface.h"

namespace viz {

SkiaOutputSurface::SkiaOutputSurface(
    scoped_refptr<ContextProvider> context_provider)
    : OutputSurface(std::move(context_provider)) {}

SkiaOutputSurface::~SkiaOutputSurface() = default;

}  // namespace viz
