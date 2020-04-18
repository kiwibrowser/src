// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_INPUT_INPUT_INJECTOR_IMPL_H_
#define CONTENT_BROWSER_FRAME_HOST_INPUT_INPUT_INJECTOR_IMPL_H_

#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/common/input/input_injector.mojom.h"

namespace content {

// An implementation of InputInjector.
class CONTENT_EXPORT InputInjectorImpl : public mojom::InputInjector {
 public:
  explicit InputInjectorImpl(base::WeakPtr<RenderFrameHostImpl> frame_host);
  ~InputInjectorImpl() override;

  static void Create(base::WeakPtr<RenderFrameHostImpl> frame_host,
                     mojom::InputInjectorRequest request);

  // mojom::InputInjector overrides.
  void QueueSyntheticSmoothDrag(
      const SyntheticSmoothDragGestureParams& drag,
      QueueSyntheticSmoothDragCallback callback) override;
  void QueueSyntheticSmoothScroll(
      const SyntheticSmoothScrollGestureParams& scroll,
      QueueSyntheticSmoothScrollCallback callback) override;
  void QueueSyntheticPinch(const SyntheticPinchGestureParams& pinch,
                           QueueSyntheticPinchCallback callback) override;
  void QueueSyntheticTap(const SyntheticTapGestureParams& tap,
                         QueueSyntheticTapCallback callback) override;
  void QueueSyntheticPointerAction(
      const SyntheticPointerActionListParams& pointer_action,
      QueueSyntheticPointerActionCallback callback) override;

 private:
  base::WeakPtr<RenderFrameHostImpl> frame_host_;

  DISALLOW_COPY_AND_ASSIGN(InputInjectorImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_INPUT_INPUT_INJECTOR_IMPL_H_
