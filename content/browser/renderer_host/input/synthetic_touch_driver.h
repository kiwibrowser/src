// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_SYNTHETIC_TOUCH_DRIVER_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_SYNTHETIC_TOUCH_DRIVER_H_

#include <array>
#include "base/macros.h"
#include "content/browser/renderer_host/input/synthetic_pointer_driver.h"
#include "content/common/content_export.h"
#include "content/common/input/synthetic_web_input_event_builders.h"

namespace content {

class CONTENT_EXPORT SyntheticTouchDriver : public SyntheticPointerDriver {
 public:
  SyntheticTouchDriver();
  explicit SyntheticTouchDriver(SyntheticWebTouchEvent touch_event);
  ~SyntheticTouchDriver() override;

  void DispatchEvent(SyntheticGestureTarget* target,
                     const base::TimeTicks& timestamp) override;

  void Press(float x,
             float y,
             int index,
             SyntheticPointerActionParams::Button button =
                 SyntheticPointerActionParams::Button::LEFT) override;
  void Move(float x, float y, int index) override;
  void Release(int index,
               SyntheticPointerActionParams::Button button =
                   SyntheticPointerActionParams::Button::LEFT) override;

  bool UserInputCheck(
      const SyntheticPointerActionParams& params) const override;

 private:
  using IndexMap = std::array<int, blink::WebTouchEvent::kTouchesLengthCap>;

  SyntheticWebTouchEvent touch_event_;
  IndexMap index_map_;

  DISALLOW_COPY_AND_ASSIGN(SyntheticTouchDriver);
};

}  // namespace content

#endif  // CONTENT_COMMON_INPUT_SYNTHETIC_TOUCH_DRIVER_H_
