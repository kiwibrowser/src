// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/input/input_injector_impl.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

void SyntheticGestureCallback(base::OnceClosure callback,
                              SyntheticGesture::Result result) {
  std::move(callback).Run();
}

}  // namespace

InputInjectorImpl::InputInjectorImpl(
    base::WeakPtr<RenderFrameHostImpl> frame_host)
    : frame_host_(std::move(frame_host)) {}

InputInjectorImpl::~InputInjectorImpl() {}

void InputInjectorImpl::Create(base::WeakPtr<RenderFrameHostImpl> frame_host,
                               mojom::InputInjectorRequest request) {
  mojo::MakeStrongBinding(std::make_unique<InputInjectorImpl>(frame_host),
                          std::move(request));
}

void InputInjectorImpl::QueueSyntheticSmoothDrag(
    const SyntheticSmoothDragGestureParams& drag,
    QueueSyntheticSmoothDragCallback callback) {
  if (!frame_host_)
    return;
  frame_host_->GetRenderWidgetHost()->QueueSyntheticGesture(
      SyntheticGesture::Create(drag),
      base::BindOnce(SyntheticGestureCallback, std::move(callback)));
}

void InputInjectorImpl::QueueSyntheticSmoothScroll(
    const SyntheticSmoothScrollGestureParams& scroll,
    QueueSyntheticSmoothScrollCallback callback) {
  if (!frame_host_)
    return;
  frame_host_->GetRenderWidgetHost()->QueueSyntheticGesture(
      SyntheticGesture::Create(scroll),
      base::BindOnce(SyntheticGestureCallback, std::move(callback)));
}

void InputInjectorImpl::QueueSyntheticPinch(
    const SyntheticPinchGestureParams& pinch,
    QueueSyntheticPinchCallback callback) {
  if (!frame_host_)
    return;
  frame_host_->GetRenderWidgetHost()->QueueSyntheticGesture(
      SyntheticGesture::Create(pinch),
      base::BindOnce(SyntheticGestureCallback, std::move(callback)));
}

void InputInjectorImpl::QueueSyntheticTap(const SyntheticTapGestureParams& tap,
                                          QueueSyntheticTapCallback callback) {
  if (!frame_host_)
    return;
  frame_host_->GetRenderWidgetHost()->QueueSyntheticGesture(
      SyntheticGesture::Create(tap),
      base::BindOnce(SyntheticGestureCallback, std::move(callback)));
}

void InputInjectorImpl::QueueSyntheticPointerAction(
    const SyntheticPointerActionListParams& pointer_action,
    QueueSyntheticPointerActionCallback callback) {
  if (!frame_host_)
    return;
  frame_host_->GetRenderWidgetHost()->QueueSyntheticGesture(
      SyntheticGesture::Create(pointer_action),
      base::BindOnce(SyntheticGestureCallback, std::move(callback)));
}

}  // namespace content
