// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_SYNTHETIC_MOUSE_DRIVER_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_SYNTHETIC_MOUSE_DRIVER_H_

#include "base/macros.h"
#include "content/browser/renderer_host/input/synthetic_pointer_driver.h"
#include "content/common/content_export.h"
#include "content/common/input/synthetic_web_input_event_builders.h"

namespace content {

class CONTENT_EXPORT SyntheticMouseDriver : public SyntheticPointerDriver {
 public:
  SyntheticMouseDriver();
  ~SyntheticMouseDriver() override;

  void DispatchEvent(SyntheticGestureTarget* target,
                     const base::TimeTicks& timestamp) override;

  void Press(float x,
             float y,
             int index = 0,
             SyntheticPointerActionParams::Button button =
                 SyntheticPointerActionParams::Button::LEFT) override;
  void Move(float x, float y, int index = 0) override;
  void Release(int index = 0,
               SyntheticPointerActionParams::Button button =
                   SyntheticPointerActionParams::Button::LEFT) override;

  bool UserInputCheck(
      const SyntheticPointerActionParams& params) const override;

 protected:
  blink::WebMouseEvent mouse_event_;

 private:
  unsigned last_modifiers_;

  DISALLOW_COPY_AND_ASSIGN(SyntheticMouseDriver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_SYNTHETIC_MOUSE_DRIVER_H_
