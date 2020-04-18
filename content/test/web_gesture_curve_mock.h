// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_WEB_GESTURE_CURVE_MOCK_H_
#define CONTENT_TEST_WEB_GESTURE_CURVE_MOCK_H_

#include <memory>

#include "base/macros.h"
#include "third_party/blink/public/platform/web_float_point.h"
#include "third_party/blink/public/platform/web_gesture_curve.h"
#include "third_party/blink/public/platform/web_size.h"

// A simple class for mocking a WebGestureCurve. The curve flings at velocity
// indefinitely.
class WebGestureCurveMock : public blink::WebGestureCurve {
 public:
  WebGestureCurveMock(const blink::WebFloatPoint& velocity,
                      const blink::WebSize& cumulative_scroll);
  ~WebGestureCurveMock() override;

  // Returns false if curve has finished and can no longer advance.
  bool Advance(double time,
               gfx::Vector2dF& out_current_velocity,
               gfx::Vector2dF& out_delta_to_scroll) override;

 private:
  blink::WebFloatPoint velocity_;
  blink::WebSize cumulative_scroll_;

  DISALLOW_COPY_AND_ASSIGN(WebGestureCurveMock);
};

#endif  // CONTENT_TEST_WEB_GESTURE_CURVE_MOCK_H_
