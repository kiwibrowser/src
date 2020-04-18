// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_UTILS_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_UTILS_H_

#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"
#include "third_party/blink/renderer/platform/transforms/transformation_matrix.h"

namespace blink {

DOMFloat32Array* transformationMatrixToFloat32Array(
    const TransformationMatrix&);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_UTILS_H_
