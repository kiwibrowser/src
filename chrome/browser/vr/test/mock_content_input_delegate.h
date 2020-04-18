// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_MOCK_CONTENT_INPUT_DELEGATE_H_
#define CHROME_BROWSER_VR_TEST_MOCK_CONTENT_INPUT_DELEGATE_H_

#include "base/macros.h"
#include "chrome/browser/vr/content_input_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/public/platform/web_gesture_event.h"

namespace vr {

class MockContentInputDelegate : public ContentInputDelegate {
 public:
  MockContentInputDelegate();
  ~MockContentInputDelegate() override;

  MOCK_METHOD1(OnHoverEnter, void(const gfx::PointF& normalized_hit_point));
  MOCK_METHOD0(OnHoverLeave, void());
  MOCK_METHOD1(OnMove, void(const gfx::PointF& normalized_hit_point));
  MOCK_METHOD1(OnButtonDown, void(const gfx::PointF& normalized_hit_point));
  MOCK_METHOD1(OnButtonUp, void(const gfx::PointF& normalized_hit_point));

  // As move-only parameters aren't supported by mock methods, we will override
  // the functions explicitly and fwd the calls to the mocked functions.
  MOCK_METHOD2(FwdContentFlingCancel,
               void(std::unique_ptr<blink::WebGestureEvent>& gesture,
                    const gfx::PointF& normalized_hit_point));
  MOCK_METHOD2(FwdContentScrollBegin,
               void(std::unique_ptr<blink::WebGestureEvent>& gesture,
                    const gfx::PointF& normalized_hit_point));
  MOCK_METHOD2(FwdContentScrollUpdate,
               void(std::unique_ptr<blink::WebGestureEvent>& gesture,
                    const gfx::PointF& normalized_hit_point));
  MOCK_METHOD2(FwdContentScrollEnd,
               void(std::unique_ptr<blink::WebGestureEvent>& gesture,
                    const gfx::PointF& normalized_hit_point));

  void OnFlingCancel(std::unique_ptr<blink::WebGestureEvent> gesture,
                     const gfx::PointF& normalized_hit_point) override {
    FwdContentFlingCancel(gesture, normalized_hit_point);
  }
  void OnScrollBegin(std::unique_ptr<blink::WebGestureEvent> gesture,
                     const gfx::PointF& normalized_hit_point) override {
    FwdContentScrollBegin(gesture, normalized_hit_point);
  }
  void OnScrollUpdate(std::unique_ptr<blink::WebGestureEvent> gesture,
                      const gfx::PointF& normalized_hit_point) override {
    FwdContentScrollUpdate(gesture, normalized_hit_point);
  }
  void OnScrollEnd(std::unique_ptr<blink::WebGestureEvent> gesture,
                   const gfx::PointF& normalized_hit_point) override {
    FwdContentScrollEnd(gesture, normalized_hit_point);
  }
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_MOCK_CONTENT_INPUT_DELEGATE_H_
