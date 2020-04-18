// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines all the public base::FeatureList features for the gpu
// module.

#ifndef GPU_CONFIG_GPU_FEATURES_H_
#define GPU_CONFIG_GPU_FEATURES_H_

#include "base/feature_list.h"
#include "gpu/gpu_export.h"

namespace features {

// All features in alphabetical order. The features should be documented
// alongside the definition of their values in the .cc file.
GPU_EXPORT extern const base::Feature kDefaultEnableGpuRasterization;

GPU_EXPORT extern const base::Feature kDefaultPassthroughCommandDecoder;

GPU_EXPORT extern const base::Feature kDirectCompositionOverlays;

}  // namespace features

#endif  // GPU_CONFIG_GPU_FEATURES_H_
