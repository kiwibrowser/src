// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_presentation_context.h"

#include "third_party/blink/renderer/bindings/modules/v8/rendering_context.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trials.h"

namespace blink {

XRPresentationContext::XRPresentationContext(
    CanvasRenderingContextHost* host,
    const CanvasContextCreationAttributesCore& attrs)
    : ImageBitmapRenderingContextBase(host, attrs) {}

XRPresentationContext::~XRPresentationContext() {}

void XRPresentationContext::SetCanvasGetContextResult(
    RenderingContext& result) {
  result.SetXRPresentationContext(this);
}

CanvasRenderingContext* XRPresentationContext::Factory::Create(
    CanvasRenderingContextHost* host,
    const CanvasContextCreationAttributesCore& attrs) {
  if (!OriginTrials::webXREnabled(host->GetTopExecutionContext()))
    return nullptr;
  return new XRPresentationContext(host, attrs);
}

}  // namespace blink
