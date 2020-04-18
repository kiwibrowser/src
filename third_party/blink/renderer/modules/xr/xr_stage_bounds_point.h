// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_STAGE_BOUNDS_POINT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_STAGE_BOUNDS_POINT_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class XRStageBoundsPoint final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  XRStageBoundsPoint(double x, double z) : x_(x), z_(z) {}

  double x() const { return x_; }
  double z() const { return z_; }

 private:
  double x_;
  double z_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_XR_XR_STAGE_BOUNDS_POINT_H_
